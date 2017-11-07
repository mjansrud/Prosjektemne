/**
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - fix deprecated convertation form string to char* startAsTag
 *  - give example description
 */
#include <SPI.h>
#include "DW1000Ranging.h"

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4; // spi select pin

void setup() {
  Serial.begin(115200);
  delay(1000);
  //init the configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  //Enable the filter to smooth the distance
  //DW1000Ranging.useRangeFilter(true);
  
  //we start the module as a tag
  //MODE_LONGDATA_RANGE_LOWPOWER
  //MODE_SHORTDATA_FAST_LOWPOWER
  //MODE_LONGDATA_FAST_LOWPOWER
  //MODE_SHORTDATA_FAST_ACCURACY
  //MODE_LONGDATA_FAST_ACCURACY
  //MODE_LONGDATA_RANGE_ACCURACY
  DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);
}

void loop() {
  DW1000Ranging.loop();
}

void newRange() {
  Serial.print("{type: 'range', device: { "); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print(", data: { distance: "); Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(", power: ");    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
  Serial.println("}}");
}

void newDevice(DW1000Device* device) {
  Serial.print("{type: 'newDevice', device: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("{type: 'inactiveDevice', device: ");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
}

