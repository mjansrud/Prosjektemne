/**
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - fix deprecated convertation form string to char* startAsAnchor
 *  - give example description
 */
#include <SPI.h>
#include "DW1000.h"
#include "DW1000Ranging.h"

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4;  // spi select pin
const uint8_t ADDRESS = "7D:00:22:EA:82:60:3B:9C";

// message system
volatile boolean received = false;
volatile boolean error = false;
String message;

// timers
unsigned long timer = 0;

// constants
const uint8_t NUM_BEACONS = 3;
const long CONFIG_TIME = 30000; //milliseconds

// states
enum STATES{
  CONFIG,  //Receive distances from other anchor and other beacons
  RANGER,  //Receive distances from other anchor and other beacons
};
STATES state = CONFIG;

// structs
struct Node{
  String device;
  uint16_t address;
  float distance;
  struct {
    float x = 0;
    float y = 0;
    float z = 0;
  } position;
};

struct Node anchor;
struct Node beacons[NUM_BEACONS];

void setup() {
  Serial.begin(115200);
  delay(1000);

  //Start receiving messages
  //Distances between beacons are needed for configuration
  initMessageConfiguration();
  
  //Update anchor with correct info
  anchor.device = "anchor";
  anchor.address = ADDRESS;

  Serial.println("Initiated anchor"); 
  serialSendPosititions();
  Serial.println("Ready to receive data from beacons");
  receiver();
  
}

void loop() {
  
  timer = millis();
  switch (state){
    case CONFIG:
       if(timer > CONFIG_TIME){
          Serial.println("Configuration complete");
          initRangeConfiguration();
          state = RANGER;
        }
         //Config state -> Receive messages
        if (received) {
          received = false;
          DW1000.getData(message);
          Serial.print("Received message #"); 
          Serial.println(message);
        }
        if (error) { 
          error = false;
          DW1000.getData(message);
          Serial.print("Error receiving message ");
          Serial.println(message);
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
  DW1000Ranging.startAsAnchor(ADDRESS, DW1000.MODE_LONGDATA_RANGE_ACCURACY);
}

 void initMessageConfiguration(){
  // Message driver configuration
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(6);
  DW1000.setNetworkId(10);
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

void serialSendPositition(struct Node node){
  Serial.print("{type: 'position', device: '"); 
  Serial.print(node.device);
  Serial.print("', address: '"); 
  Serial.print(node.address, HEX);
  Serial.print("', data: { x: ");
  Serial.print(node.position.x); Serial.print(", y: ");
  Serial.print(node.position.y); Serial.print(", z: ");
  Serial.print(node.position.z);
  Serial.println("}}");
}

void serialSendPosititions(){
  serialSendPositition(anchor);
  for (int i = 0; i< NUM_BEACONS; i++){
    serialSendPositition(beacons[i]);
  }
}

void initBeacons(){
  for (int i = 0; i< NUM_BEACONS; i++){
    struct Node beacon;
    beacon.device = "beacon" + i;
    beacon.address = 0;
    beacons[i] = beacon;
  }
}

void addBeacon(uint16_t address){
  bool exists = false;
  for (int i = 0; i < NUM_BEACONS; i++){
    if(beacons[i].address == address){
      exists = true;
      break;
    }
  }
  if(!exists){
     for (int i = 0; i < NUM_BEACONS; i++){
      if(!beacons[i].address){
        beacons[i].address = address;
        break;
      }
    }
  }
  serialSendPosititions();
  
}

void removeBeacon(uint16_t address){
  for (int i = 0; i < NUM_BEACONS; i++){
    if(beacons[i].address == address){
      beacons[i].address = 0;
      beacons[i].distance = 0;
      beacons[i].position.x = 0;
      beacons[i].position.y = 0;
      beacons[i].position.z = 0;
      break;
    }
  }
  serialSendPosititions();
}

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
  Serial.print("{type: 'range', device: '"); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print("', data: { distance: "); Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(", power: ");    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
  Serial.println("}}");
}

void interuptNewBlink(DW1000Device* device) {
  Serial.print("{type: 'newDevice', device: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
  addBeacon(device->getShortAddress());
}

void interuptInactiveDevice(DW1000Device* device) {
  Serial.print("{type: 'inactiveDevice', device: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
  removeBeacon(device->getShortAddress());
}



