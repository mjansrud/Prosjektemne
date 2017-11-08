/**
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - fix deprecated convertation form string to char* startAsTag
 *  - give example description
 */
#include <SPI.h>
#include <DW1000.h>
#include "DW1000Ranging.h"

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4; // spi select pin
const uint8_t ADDRESS = "7D:00:22:EA:82:60:3B:9C";

// variables for message system
volatile boolean sending = false;

// timers
unsigned long timer = 0;

// states
bool configuration = false;
bool ready = false;

// constants
const long CONFIG_TIME = 10000; //milliseconds
const long INIT_POLLING_TIME = 10000; //milliseconds

enum STATES{
  CONFIG,   //Receive distances from other anchor and other beacons
  SENDER,   //Send the distances to the beacon
  RANGER,   //Send the distances to the beacon
};

STATES state = CONFIG;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  //Start receiving distances from anchor and other beacons 
  initRangeConfiguration();

}

void loop() {
  timer = millis();

  switch (state){
    case CONFIG:
      if(timer > INIT_POLLING_TIME){
        initMessageConfiguration();
        configuration = true;
        state = SENDER;
      }
    break;    
    case SENDER:
      if(timer > (INIT_POLLING_TIME + CONFIG_TIME)){
        initRangeConfiguration();
        configuration = false;
        state = RANGER;
      }
      sendMessage();
      break;
    default:
    break;
  }

  switch (state){
    case CONFIG:
    case RANGER:
      DW1000Ranging.loop();
    break;
  }
  
}
/*
 * Configuration
 */

void initRangeConfiguration(){
  // Ranging driver configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  DW1000Ranging.startAsTag(ADDRESS, DW1000.MODE_LONGDATA_RANGE_ACCURACY);
}

void initMessageConfiguration(){
  // Message driver configuration
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(5);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();
  
  //Attach message interrupts  
  DW1000.attachSentHandler(handleSent);
}
/*
 * Messages
 */

void sendMessage() {
  DW1000.newTransmit();
  DW1000.setDefaults();
  sending = true;

  //Make message
  String msg = "Hello DW1000, it's #"; msg += String(ADDRESS);
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
 */
 
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

