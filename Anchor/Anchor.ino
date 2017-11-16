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
const uint8_t NETWORK_DEVICE_ADDRESS = 2;
const uint8_t NETWORK_ID = 10;

// other
const uint8_t NUM_DEVICES = 4; 
const long INIT_POLLING_TIME = 40000; //milliseconds
const long CONFIG_TIME = 20000;       //milliseconds
const float STANDARD_DEVIATION = -0.5;
bool TEMP_TAG = false;

// variables for message system
volatile boolean sending = false;

// timers
unsigned long time_current = 0;
unsigned long time_start = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  time_start = millis();
  
  //Distances between beacons are needed for configuration
  DW1000RangingInitConfiguration(ANCHOR);
  
  //Start receiving distances from anchor and other beacons 
  DW1000Positioning.initDevices();
  DW1000Positioning.startAsAnchor(NETWORK_DEVICE_ADDRESS);
  DW1000Positioning.serialDrawDistances();
  
  Serial.print("Initiated anchor -> address "); 
  Serial.println(NETWORK_DEVICE_ADDRESS); 
  Serial.println("Ready to receive data from tag");
  
}

void loop() {
  time_current = millis() - time_start;

  switch (DW1000Positioning.getState()){
     case CONFIG:
      DW1000Ranging.loop();

      if(time_current > (INIT_POLLING_TIME / NUM_DEVICES) * (NETWORK_DEVICE_ADDRESS) && !TEMP_TAG){
        Serial.println("Temporarily becoming tag");
        DW1000RangingInitConfiguration(TAG);
        TEMP_TAG = true;
      }
      
      if(time_current > (INIT_POLLING_TIME / NUM_DEVICES) * (NETWORK_DEVICE_ADDRESS + 1) && TEMP_TAG){
        Serial.println("Resetting to anchor");
        DW1000RangingInitConfiguration(ANCHOR);
        TEMP_TAG = false;
      }

      //Switch state
      if(time_current > INIT_POLLING_TIME){
        Serial.println("Ready to send data to tag");
        DW1000MessagingInitConfiguration();
        DW1000Positioning.setState(SENDER);
      }
    break;   
    case SENDER:
      if(!sending){
        sendMessage();
        delay(100);
      }
      
      //Switch state
      if(time_current > (INIT_POLLING_TIME + CONFIG_TIME)){
        DW1000RangingInitConfiguration(ANCHOR);
        DW1000Positioning.setState(RANGING);
      }
      break;
    case RANGING:
      DW1000Ranging.loop();
      break;
    default:
      break;
  }
  
}
/*
 * Configuration
 */

void DW1000RangingInitConfiguration(uint8_t type){
  // Ranging driver configuration 
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  DW1000Ranging.attachNewRange(interuptNewRange);
  DW1000Ranging.attachNewDevice(interuptNewDevice);
  DW1000Ranging.attachInactiveDevice(interuptInactiveDevice);

  //We need to temporarily become tag to receive distances between anchors
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
  DW1000.attachSentHandler(handleSent);
}

/*
 * Messages
 */

void sendMessage() {
  sending = true;
  DW1000.newTransmit();
  DW1000.setDefaults();

  //Make message
  String msg = DW1000Positioning.createJsonDistance(DW1000Positioning.getNextDevice());
  Serial.print("Transmitting '"); 
  Serial.print(msg);
  Serial.println("'");
  DW1000.setData(msg);
  
  // delay sending the message for the given amount
  DW1000Time deltaTime = DW1000Time(10, DW1000Time::MILLISECONDS);
  DW1000.setDelay(deltaTime);
  DW1000.startTransmit();
}

void handleSent() {
  // status change on sent success
  sending = false;
}

/*
 * Ranges
 * 
 */
 
void interuptNewRange() {
  uint8_t address = (DW1000Ranging.getDistantDevice()->getShortAddress() / 1U) % 10;
  float range = DW1000Ranging.getDistantDevice()->getRange() + STANDARD_DEVIATION;
  float power = DW1000Ranging.getDistantDevice()->getRXPower();
  Serial.print("From:"); Serial.print(address);
  Serial.print(",range:"); Serial.print(range);
  Serial.print(",pow:"); Serial.println(power); 
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




