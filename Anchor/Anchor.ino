/**
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - fix deprecated convertation form string to char* startAsAnchor
 *  - give example description
 */
#include <SPI.h>
#include "DW1000Ranging.h"

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4; // spi select pin

//Nodes
const uint8_t NUM_NODES = 4; // spi select pin
DW1000Device* beacons[NUM_NODES * sizeof(DW1000Device)];
struct {
  struct {
    float x = 0;
    float y = 0;
    float z = 0;
  } position;
} anchor;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  //init the configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); 
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachBlinkDevice(newBlink);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  
  //Enable the filter to smooth the distance
  //DW1000Ranging.useRangeFilter(true);
  
  //we start the module as an anchor
  DW1000Ranging.startAsAnchor("82:17:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);

  //Printing
  printf("Initiating anchor with position [%f, %f, %f]", anchor.position.x, anchor.position.y, anchor.position.z);
  
}

void loop() {
  DW1000Ranging.loop();
}

int calculatePosition(){
/*
  int distance_beacon_1 = norm(my_pos - node_1_pos)
  int distance_beacon_2 = norm(my_pos - node_2_pos)
  int distance_beacon_3 = norm(my_pos - node_3_pos)
  */
}

void newRange() {
  Serial.print("{type: 'range', device: { "); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print(", data: { distance: "); Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(", power: ");    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
  Serial.println("}}");
}

void newBlink(DW1000Device* device) {
  Serial.print("{type: 'newDevice', device: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("{type: 'inactiveDevice', device: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
}


