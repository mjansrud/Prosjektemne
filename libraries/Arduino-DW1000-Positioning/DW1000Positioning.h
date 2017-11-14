/*
 * Fader
 * Version 0.1 October, 2015
 * Copyright 2015 Alan Zucconi
 * wwww.alanzucconi.com
 *
 */

#ifndef DW1000Positioning_h
#define DW1000Positioning_h

#include <string.h>
#include <arduino.h>

// structs
struct _Node{
    String type;
    bool active;
    uint16_t address = 0;
    float distance = 0.0;
    struct {
        float x = 0.0;
        float y = 0.0;
        float z = 0.0;
    } position;
};

// states
enum _STATES{
    CONFIG,
    SENDER,         //Anchors -> Send all distances to tah
    RECEIVER,       //Tag -> Receive all distances from Anchors, parallell with SENDER
    RANGING,        //Nodes receives distances from other nodes
    INVALID,
};

// states
enum _MESSAGE_TYPE{
    POSITION,
    DISTANCE,
};

// conts
#define _NUM_DEVICES 4

// Results returned from the decoder
class DW1000PositioningClass {
  public:
    
    void DW1000PositioningClass::startAsAnchor(uint8_t address);
    void DW1000PositioningClass::startAsTag(uint8_t address);
    void DW1000PositioningClass::initTestDevices();
    void DW1000PositioningClass::initDevices();
    void DW1000PositioningClass::activeDevice(uint8_t address);
    void DW1000PositioningClass::inactiveDevice(uint8_t address);
    void DW1000PositioningClass::setState(_STATES state);
    void DW1000PositioningClass::serialSendPosititions();
    void DW1000PositioningClass::serialSendPositition(struct _Node _node);
    void DW1000PositioningClass::serialSendDistances();
    void DW1000PositioningClass::serialSendDistance(struct _Node _node);
    void DW1000PositioningClass::loop();
    void DW1000PositioningClass::setDistance(uint8_t _address, float _distance);
    struct _Node DW1000PositioningClass::getNextDevice();
    struct _Node DW1000PositioningClass::getDevice();
    String DW1000PositioningClass::createJsonPosition(struct _Node node);
    String DW1000PositioningClass::createJsonDistance(struct _Node node);
    String DW1000PositioningClass::createJsonPositions();
    String DW1000PositioningClass::createJsonDistances();
    _STATES DW1000PositioningClass::getState();

  private:
    
    _STATES _state = CONFIG;
    int _nextDevice = 0;
    bool _isTag = true;
    struct _Node _device;
    struct _Node _devices[_NUM_DEVICES];
    
};

extern DW1000PositioningClass DW1000Positioning;

#endif
