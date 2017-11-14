/*
 * Fader
 * Version 0.1 October, 2015
 * Copyright 2015 Alan Zucconi
 *
 */


#include "DW1000Positioning.h"

DW1000PositioningClass DW1000Positioning;

void DW1000PositioningClass::startAsAnchor(uint8_t _address){
    
    initDevices();
    
    _isTag = false;
    _device = _devices[_address];
    
}

void DW1000PositioningClass::startAsTag(uint8_t _address){
    
    initDevices();
    
    _isTag = true;
    _device = _devices[_address];
    
}

void DW1000PositioningClass::loop(){
    
    
}

void DW1000PositioningClass::initTestDevices(){
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        struct _Node _node;
        if(i == 0){
            _node.type = "tag";
        }else{
            _node.type = "anchor";
        }
        _node.address = i;
        _node.distance = random(2, 100);
        _devices[i] = _node;
    }
}


void DW1000PositioningClass::initDevices(){
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        struct _Node _node;
        if(i == 0){
            _node.type = "tag";
        }else{
            _node.type = "anchor";
        }
        _node.address = i;
        _node.active = false;
        _devices[i] = _node;
    }
}

void DW1000PositioningClass::activeDevice(uint8_t _address){
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        if(_devices[i].address == _address){
            _devices[i].active = true;
            break;
        }
    }
    serialSendDistances();
}

void DW1000PositioningClass::inactiveDevice(uint8_t _address){
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        if(_devices[i].address == _address){
            _devices[i].active = false;
            _devices[i].distance = 0.0;
            _devices[i].position.x = 0.0;
            _devices[i].position.y = 0.0;
            _devices[i].position.z = 0.0;
            break;
        }
    }
}

void DW1000PositioningClass::setDistance(uint8_t _address, float _distance){
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        if(_devices[i].address == _address){
            _devices[i].distance = _distance;
            break;
        }
    }
}

String DW1000PositioningClass::createJsonPosition(struct _Node _node){
    /*
        TODO: Send device information more effiecient than json
    */
    String json;
    json += "{type:";
    json += POSITION;
    json += ",device:'";
    json += _node.type;
    json += "',address:'";
    json += String(_node.address, HEX);
    json += "',data:{x:";
    json += _node.position.x;
    json += ",y:";
    json += _node.position.y;
    json += ",z:";
    json += _node.position.z;
    json += "}}";
    return json;
}

String DW1000PositioningClass::createJsonDistance(struct _Node _node){
    /*
     TODO: Send device information more effiecient than json
     */
    String json;
    json += "{type:";
    json += DISTANCE;
    json += ",from:";
    json += _device.address;
    json += ",to:";
    json += _node.address;
    json += ",distance:";
    json += _node.distance;
    json += "}"; 
    return json;
}


String DW1000PositioningClass::createJsonDistances(){
    /*
     TODO: Send device information more effiecient than json
     */
    String json;
    json += "[";
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        if(_device.address != _devices[i].address){
            json += createJsonDistance(_devices[i]);
            if(i !=_NUM_DEVICES - 1){
                json += ",";
            }
        }
    }
    json += "]";
    return json;

}

String DW1000PositioningClass::createJsonPositions(){
    String json;
    json += "[";
    for (uint8_t i = 0; i < _NUM_DEVICES; i++){
        if(_device.address != _devices[i].address){
            json += createJsonPosition(_devices[i]);
            if(i !=_NUM_DEVICES - 1){
                json += ",";
            }
        }
    }
    json += "]";
    return json;
}

void DW1000PositioningClass::serialSendPositition(struct _Node _node){
    Serial.println(createJsonPosition(_node));
}

void DW1000PositioningClass::serialSendPosititions(){
    Serial.println(createJsonPositions());
}

void DW1000PositioningClass::serialSendDistance(struct _Node _node){
    Serial.println(createJsonDistance(_node));
}

void DW1000PositioningClass::serialSendDistances(){
    Serial.println(createJsonDistances());
}

/*
    Setters and getters
*/

struct _Node DW1000PositioningClass::getNextDevice(){
    _nextDevice == 2 ? _nextDevice = 0 : _nextDevice++;
    return _devices[_nextDevice];
}

struct _Node DW1000PositioningClass::getDevice(){
    return _device;
}

_STATES DW1000PositioningClass::getState(){
    return _state;
}

void DW1000PositioningClass::setState(_STATES state){
    _state = state;
}
 
