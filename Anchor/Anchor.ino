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
const uint8_t PIN_SS = 4;  // spi select pin

const uint8_t NETWORK_DEVICE_ADDRESS = 2;
const uint8_t NETWORK_ID = 10;

// variables for message system
volatile boolean sending = false;

// timers
unsigned long timer = 0;

// constants
const uint8_t NUM_DEVICES = 4; 
const long INIT_POLLING_TIME = 40000; //milliseconds
const long CONFIG_TIME = 20000;       //milliseconds
bool TEMP_TAG = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  //Distances between beacons are needed for configuration
  DW1000RangingInitConfiguration(ANCHOR);
  
  //Start receiving distances from anchor and other beacons 
  DW1000Positioning.startAsAnchor(NETWORK_DEVICE_ADDRESS);
  DW1000Positioning.serialSendDistances(); 
  
  Serial.print("Initiated anchor -> address "); 
  Serial.println(NETWORK_DEVICE_ADDRESS); 
  Serial.println("Ready to receive data from tag");
  
}

void loop() {
  timer = millis();

  switch (DW1000Positioning.getState()){
     case CONFIG:
      DW1000Ranging.loop();

      if(timer > (INIT_POLLING_TIME / NUM_DEVICES) * (NETWORK_DEVICE_ADDRESS) && !TEMP_TAG){
        Serial.println("Temporarily becoming tag");
        DW1000RangingInitConfiguration(TAG);
        TEMP_TAG = true;
      }
      
      if(timer > (INIT_POLLING_TIME / NUM_DEVICES) * (NETWORK_DEVICE_ADDRESS + 1) && !TEMP_TAG){
        Serial.println("Resetting to anchor");
        DW1000RangingInitConfiguration(ANCHOR);
        TEMP_TAG = false;
      }

      //Switch state
      if(timer > INIT_POLLING_TIME){
        Serial.println("Ready to send data to anchor");
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
      if(timer > (INIT_POLLING_TIME + CONFIG_TIME)){
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
  Serial.print("{type:'range',device:"); Serial.print(address);
  Serial.print(",distance:"); Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(",power:"); Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); 
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


