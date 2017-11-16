/**
 * Written by Morten Jansrud
 */
 
#include <SPI.h>
#include <DW1000.h>
#include <DW1000Ranging.h>
#include <DW1000Positioning.h>
#include <ArduinoJson.h>

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4;  // spi select pin

// network
const uint8_t NETWORK_DEVICE_ADDRESS = 0;
const uint8_t NETWORK_ID = 10;

// other
const uint8_t NUM_DEVICES = 4; 
const long DRAW_INTERVAL = 5000;      //milliseconds
const long INIT_POLLING_TIME = 40000; //milliseconds
const long CONFIG_TIME = 20000;       //milliseconds
const float STANDARD_DEVIATION = -0.5;
bool TEMP_ANCHOR = false;

// message system
String message;
volatile boolean received = false;
volatile boolean error = false;

// timers
unsigned long time_current = 0;
unsigned long time_start = 0;
 unsigned long time_last_draw = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  time_start = millis();
  
  //Distances between beacons are needed for configuration
  DW1000RangingInitConfiguration(TAG);
  
  //Update anchor with correct info
  DW1000Positioning.initDevices();
  DW1000Positioning.startAsTag(NETWORK_DEVICE_ADDRESS);
  DW1000Positioning.calculatePositionsAndDraw();
  
  Serial.print("Initiated tag -> Address "); 
  Serial.println(NETWORK_DEVICE_ADDRESS); 

}

void loop() {
  time_current = millis() - time_start;
  
  switch (DW1000Positioning.getState()){  
    case CONFIG:
      DW1000Ranging.loop();

      if(time_current > (INIT_POLLING_TIME / NUM_DEVICES) && !TEMP_ANCHOR){
        Serial.println("Temporarily becoming anchor");
        DW1000RangingInitConfiguration(ANCHOR);
        DW1000Positioning.calculatePositionsAndDraw();
        TEMP_ANCHOR = true;
      } 
      
      //Switch state
      if(time_current > INIT_POLLING_TIME){
        Serial.println("Ready to receive data from beacons");
        DW1000MessagingInitConfiguration();
        DW1000Positioning.setState(RECEIVER);
        DW1000Positioning.calculatePositionsAndDraw();
        // start a receiver
        receiver();        
      }
    break;    
    case RECEIVER:
    
      //Config state -> Receive messages
      if (received) {
        
        received = false;
        DW1000.getData(message);
        
        const size_t bufferSize = JSON_OBJECT_SIZE(4);
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& msg = jsonBuffer.parseObject(message);      

        uint8_t from = msg["from"];
        uint8_t to = msg["to"];
        float range = msg["range"];
        
        DW1000Positioning.setDistanceBetweenDevices(from, to, range);
       
      } 

      //Switch state
      if(time_current > (INIT_POLLING_TIME + CONFIG_TIME)){
          //Reset timer
          time_last_draw = millis();

          //Done receiving messages, reset to tag
          DW1000RangingInitConfiguration(TAG);
          DW1000Positioning.setState(RANGING);
          DW1000Positioning.calculatePositionsAndDraw();
      }
    break;
    case RANGING:
      //Ranging state -> Check distances to nodes
      DW1000Ranging.loop();

      if(millis() - time_last_draw >= DRAW_INTERVAL){
        time_last_draw += DRAW_INTERVAL;
        DW1000Positioning.calculatePositionsAndDraw();
      }
    break;
  }
  
}

/*
 * Configurations
 */

void DW1000RangingInitConfiguration(uint8_t type){
  // Ranging driver configuration 
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  DW1000Ranging.attachNewRange(interuptNewRange);
  DW1000Ranging.attachNewDevice(interuptNewDevice);
  DW1000Ranging.attachInactiveDevice(interuptInactiveDevice);

  //We need to temporarily become anchor to let anchors receive distances between each other
  if(type == ANCHOR){
      DW1000Ranging.startAsAnchor(NETWORK_DEVICE_ADDRESS, DW1000.MODE_LONGDATA_RANGE_ACCURACY, NETWORK_DEVICE_ADDRESS);
  }else{
      DW1000Ranging.startAsTag(NETWORK_DEVICE_ADDRESS, DW1000.MODE_LONGDATA_RANGE_ACCURACY, NETWORK_DEVICE_ADDRESS);
  }
}

 void DW1000MessagingInitConfiguration(){
  
  // Message driver configuration
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(NETWORK_DEVICE_ADDRESS);
  DW1000.setNetworkId(NETWORK_ID);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  //Attach message interrupts  
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveFailedHandler(handleError);
  DW1000.attachErrorHandler(handleError);
}

/*
 * Message system
 */

void handleReceived() {
  // status change on reception success
  received = true;
}

void handleError() {
  error = true;
}

void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

//Interupts 
void interuptNewRange() {
  uint8_t address = (DW1000Ranging.getDistantDevice()->getShortAddress() / 1U) % 10;
  float range = DW1000Ranging.getDistantDevice()->getRange() + STANDARD_DEVIATION;
  float power = DW1000Ranging.getDistantDevice()->getRXPower();
  Serial.print("Dev:"); Serial.print(address);
  Serial.print(",range:"); Serial.print(range);
  Serial.print(",power:"); Serial.println(power);
  DW1000Positioning.setDistance(address, range);
}

void interuptNewDevice(DW1000Device* device) { 
  uint8_t address = (device->getShortAddress() / 1U) % 10;
  Serial.print("Active device, address:");
  Serial.println(address);
  DW1000Positioning.activeDevice(address);
}

void interuptInactiveDevice(DW1000Device* device) {
  uint8_t address = (device->getShortAddress() / 1U) % 10;
  Serial.print("Inactive device, address:");
  Serial.println(address);
  if(DW1000Positioning.getState() == RANGING){
    DW1000Positioning.inactiveDevice(address);
  }
}



