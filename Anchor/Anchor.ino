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
const uint8_t ADDRESS = 6;
const uint8_t NETWORK_ID = 10;

// message system
String message;
volatile boolean received = false;
volatile boolean error = false;

// timers
unsigned long timer = 0;

// constants
const long INIT_POLLING_TIME = 5000; //milliseconds
const long CONFIG_TIME = 20000;       //milliseconds

void setup() {
  Serial.begin(115200);
  delay(1000);

  //Distances between beacons are needed for configuration
  initRangeConfiguration();
  
  //Update anchor with correct info
  DW1000Positioning.startAsAnchor(ADDRESS);

  Serial.print("Initiated anchor -> Address "); 
  Serial.println(ADDRESS); 
  Serial.println("Polling distance to beacons");
  
}

void loop() {
  
  timer = millis();
  switch (DW1000Positioning.getState()){
    case CONFIG:
      DW1000Ranging.loop();

      //Switch state
      if(timer > INIT_POLLING_TIME){
        Serial.println("Ready to receive data from beacons");
        initMessageConfiguration();
        DW1000Positioning.setState(RECEIVER);
        // start a receiver
        receiver();        
      }
    break;    
    case RECEIVER:
      if(timer > (INIT_POLLING_TIME + CONFIG_TIME)){
          initRangeConfiguration();
          DW1000Positioning.setState(RANGER);
      }
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
    break;
    case RANGER:
      //Ranging state -> Check distances to nodes
      DW1000Ranging.loop();
    break;
  }
  
}

/*
 * Configurations
 */

 void initRangeConfiguration(){
  // Ranging driver configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); 
  DW1000Ranging.attachNewRange(interuptNewRange);
  DW1000Ranging.attachBlinkDevice(interuptNewBlink);
  DW1000Ranging.attachInactiveDevice(interuptInactiveDevice);
  DW1000Ranging.startAsAnchor("82:17:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);
}

 void initMessageConfiguration(){
  // Message driver configuration
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(ADDRESS);
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

void addDistance(uint16_t address){

}

int calculatePosition(){
/*
  int distance_beacon_1 = norm(my_pos - node_1_pos)
  int distance_beacon_2 = norm(my_pos - node_2_pos)
  int distance_beacon_3 = norm(my_pos - node_3_pos)
  */
}

//Interupts
void interuptNewRange() {
  Serial.print("{type:'range',address:'"); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print("',data:{ distance:"); Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(",power:");    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
  Serial.println("}}");
  DW1000Positioning.setDistance(DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange());

}

void interuptNewBlink(DW1000Device* device) {
  Serial.print("{type:'newDevice',address:");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
  DW1000Positioning.addNetworkDevice(device->getShortAddress());
}

void interuptInactiveDevice(DW1000Device* device) {
  Serial.print("{type:'inactiveDevice',address:");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
  DW1000Positioning.removeNetworkDevice(device->getShortAddress());
}


