/**
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - fix deprecated convertation form string to char* startAsAnchor
 *  - give example description
 */
 
#include <SPI.h>
#include <DW1000.h>
#include <DW1000Ranging.h>
#include <DW1000Positioning.h>

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4;  // spi select pin

const uint8_t NETWORK_DEVICE_ADDRESS = 0;
const uint8_t NETWORK_ID = 10;

// message system
String message;
volatile boolean received = false;
volatile boolean error = false;

// timers
unsigned long timer = 0;

// constants
const uint8_t NUM_DEVICES = 4; 
const long INIT_POLLING_TIME = 40000; //milliseconds
const long CONFIG_TIME = 20000;       //milliseconds
bool TEMP_ANCHOR = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  //Distances between beacons are needed for configuration
  DW1000RangingInitConfiguration(TAG);
  
  //Update anchor with correct info
  DW1000Positioning.startAsTag(NETWORK_DEVICE_ADDRESS);
  DW1000Positioning.serialSendDistances();

  Serial.print("Initiated tag -> Address "); 
  Serial.println(NETWORK_DEVICE_ADDRESS); 
  Serial.println("Polling distance to anchors");
  
}

void loop() {
  
  timer = millis();
  switch (DW1000Positioning.getState()){  
    case CONFIG:
      DW1000Ranging.loop();

      if(timer > (INIT_POLLING_TIME / NUM_DEVICES) && !TEMP_ANCHOR){
        Serial.println("Temporarily becoming anchor");
        DW1000RangingInitConfiguration(ANCHOR);
        TEMP_ANCHOR = true;
      } 
      
      //Switch state
      if(timer > INIT_POLLING_TIME){
        Serial.println("Ready to receive data from beacons");
        DW1000MessagingInitConfiguration();
        DW1000Positioning.setState(RECEIVER);
        DW1000Positioning.serialSendDistances(); 
        // start a receiver
        receiver();        
      }
    break;    
    case RECEIVER:
    
      //Config state -> Receive messages
      if (received) {
        received = false;
        DW1000.getData(message);
        Serial.print("Received '"); 
        Serial.print(message);
        Serial.println("'"); 
      }
      if (error) { 
        error = false;
        DW1000.getData(message);
        Serial.print("Error receiving '"); 
        Serial.print(message);
        Serial.println("'"); 
      }

      //Switch state
      if(timer > (INIT_POLLING_TIME + CONFIG_TIME)){
          DW1000RangingInitConfiguration(TAG);
          DW1000Positioning.setState(RANGING);
      }
    break;
    case RANGING:
      //Ranging state -> Check distances to nodes
      DW1000Ranging.loop();
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

 /*
  * Position/range
*/

int calculatePosition(){
/*
  int distance_beacon_1 = norm(my_pos - node_1_pos)
  int distance_beacon_2 = norm(my_pos - node_2_pos)
  int distance_beacon_3 = norm(my_pos - node_3_pos)
  */
}

//Interupts 
void interuptNewRange() {
  uint8_t address = (DW1000Ranging.getDistantDevice()->getShortAddress() / 1U) % 10;
  Serial.print("{type:'range',device:");  
  Serial.print(address);
  Serial.print(",distance:"); 
  Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(",power:");    
  Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
  Serial.println("}");
  DW1000Positioning.setDistance(address, DW1000Ranging.getDistantDevice()->getRange());
}

void interuptNewDevice(DW1000Device* device) { 
  uint8_t address = (device->getShortAddress() / 1U) % 10;
  Serial.print("{type:'activeDevice',device:");
  Serial.print(address);
  Serial.println("}"); 
  DW1000Positioning.activeDevice(address);
}

void interuptInactiveDevice(DW1000Device* device) {
  uint8_t address = (device->getShortAddress() / 1U) % 10;
  Serial.print("{type:'inactiveDevice',device:");
  Serial.print(address);
  Serial.println("}");
  if(DW1000Positioning.getState() == RANGING){
    DW1000Positioning.inactiveDevice(address);
  }
}


