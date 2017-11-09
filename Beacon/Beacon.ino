/**
 * 
 * @todo
 *  - move strings to flash (less RAM consumption)
 *  - fix deprecated convertation form string to char* startAsTag
 *  - give example description
 */
#include <SPI.h>
#include <DW1000.h>
#include <DW1000Ranging.h>
#include <DW1000Positioning.h>

// connection pins
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4; // spi select pin
const uint8_t ADDRESS = 5;
const uint8_t NETWORK_ID = 10;

// variables for message system
volatile boolean sending = false;

// timers
unsigned long timer = 0;

// constants
const long INIT_POLLING_TIME = 5000; //milliseconds
const long CONFIG_TIME = 5000;       //milliseconds
const uint8_t NUM_BEACONS = 3;

void setup() {
  Serial.begin(115200);
  delay(1000);

  //Distances between beacons are needed for configuration
  initRangeConfiguration();
  
  //Start receiving distances from anchor and other beacons 
  DW1000Positioning.startAsBeacon(ADDRESS);
  
  Serial.print("Initiated beacon -> address "); 
  Serial.println(ADDRESS); 
  Serial.println("Ready to receive data from beacons");
  
}

void loop() {
  timer = millis();

  switch (DW1000Positioning.getState()){
    case CONFIG:
      DW1000Ranging.loop();

      //Switch state
      if(timer > INIT_POLLING_TIME){
        Serial.println("Ready to send data to anchor");
        initMessageConfiguration();
        DW1000Positioning.setState(SENDER);
      }
    break;    
    case SENDER:
      if(!sending){
        sendMessage();
        delay(100);
      }
      
      //Switch state
      if(timer > (INIT_POLLING_TIME + CONFIG_TIME)){
        initRangeConfiguration();
        DW1000Positioning.setState(RANGER);
      }
      break;
    case RANGER:
      DW1000Ranging.loop();
      break;
    default:
      break;
  }
  
}
/*
 * Configuration
 */

void initRangeConfiguration(){
  // Ranging driver configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  DW1000Ranging.attachNewRange(newDistance);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);
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
 */
 
void newDistance() {
  Serial.print("{type:'range',device:{"); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print(",data:{distance:"); Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(",power:");    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
  Serial.println("}}");
  DW1000Positioning.setDistance(DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange());
}

void newDevice(DW1000Device* device) {
  Serial.print("{type:'newDevice',device:");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
  DW1000Positioning.addNetworkDevice(device->getShortAddress());
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("{type:'inactiveDevice',device:");
  Serial.print(device->getShortAddress(), HEX);
  Serial.println("}");
  //DW1000Positioning.removeNetworkDevice(device->getShortAddress());
}


