/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * maybe
 *
 * DESCRIPTION
 * coming soon
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <MySensors.h>

#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)

const byte voltageMeasurePin = A7;
const float voltsPerADCStep = 0.0146;

const byte dimmerOutputs[] = {3}; //has to be transistor/mosfet
const byte powerOutputs[] = {4, 5, 6, 7}; //pins with relays/mosfets connected to them to switch outputs
const byte buttons[] = {A0, A1, A2, A3, A4}; //maps to all dimmers then to all outputs in ascending order

const byte dimmerChannelOffset = 4; //move digital pins behind in the channel mapping (for expandability)
const byte powerChannelOffset = 8; 

byte dimmerLevel[sizeof(dimmerOutputs)], targetDimValue[sizeof(dimmerOutputs)];
bool fadeRunning[sizeof(dimmerOutputs)];
bool outputState[sizeof(powerOutputs)];
bool lastBtnState[sizeof(buttons)];

byte debounceTime = 40;

void setup()
{
    for(byte i = 0; i < sizeof(buttons); i++) {
        pinMode(buttons[i], INPUT_PULLUP);
        lastBtnState[i] = 1;
    }
    for(byte i = 0; i < sizeof(powerOutputs); i++) { //sizeof reports size in bytes
        //outputState[i] = request(i + powerChannelOffset, V_STATUS); //currently does not work
        pinMode(powerOutputs[i], OUTPUT);
        setOutput(i, 0);
    }
    for(byte i = 0; i < sizeof(dimmerOutputs); i++) { //sizeof reports size in bytes
        //byte level = request(i + dimmerChannelOffset, V_PERCENTAGE); //currently does not work
        pinMode(dimmerOutputs[i], OUTPUT);
        fadeToLevel(i, /*level*/0);
    }
}

void presentation()
{
    // Register the LED Dimmable Light with the gateway
    present(0, S_MULTIMETER);
    present(4, S_DIMMER);
    for(byte i = 0; i < sizeof(powerOutputs); i++) { //sizeof reports size in bytes
        present(i + powerChannelOffset, S_BINARY);
    }
    
	sendSketchInfo("12V Distribution Box 01", "0.0.1");
}


unsigned long lastFadeTick = 0;

void loop()
{
    if(millis() - lastFadeTick >= FADE_DELAY) {
        lastFadeTick = millis();
        fadeTick();
    }
    handleButtons();
}

unsigned long lastBtnPress = 0;

void handleButtons() { //TODO: Dimmer button, no head for it right now :P
    for(byte i = 0; i < sizeof(buttons); i++) {
        bool state = digitalRead(buttons[i]);
        if(state != lastBtnState[i]) {
            lastBtnState[i] = state;
            if(millis() - lastBtnPress > debounceTime) {
                if(!state) {
                    toggleOutput(i - sizeof(dimmerOutputs)); //button mapping
                }
            }
        }
    }
}


void receive(const MyMessage &message)
{
    if(message.getCommand() == C_SET) {
        if(message.type == V_STATUS) {
            bool status = message.getBool();
            if(message.sensor >= powerChannelOffset) {
                byte outputId = message.sensor - powerChannelOffset;
                setOutput(outputId, status);
            }
            else if(message.sensor >= dimmerChannelOffset) {
                byte outputId = message.sensor - dimmerChannelOffset;
                fadeToLevel(outputId, status * 100);
            }
            MyMessage stat(message.sensor, V_STATUS); //hopefully won't cause an overflow
            send(stat.set(status));
        }
        else if(message.type == V_PERCENTAGE) {
            byte id = message.sensor;
            if(id >= dimmerChannelOffset && id < powerChannelOffset) {
                byte outputId = message.sensor - dimmerChannelOffset;
                byte level = message.getUInt();
                fadeToLevel(outputId, level);
                    
                MyMessage dim(message.sensor, V_DIMMER); //hopefully won't cause an overflow
                MyMessage stat(message.sensor, V_STATUS);
                send(stat.set(level > 0));
                send(dim.set(level));
            }
        }
    }
    else if(message.getCommand() == C_REQ) { //answer requests
        byte id = message.sensor;
        if(id >= powerChannelOffset) {
            MyMessage m(id, V_STATUS);
            send(m.set(outputState[id - powerChannelOffset]));
        }
        else if(id >= dimmerChannelOffset) {
            MyMessage dim(message.sensor, V_DIMMER);
            MyMessage stat(message.sensor, V_STATUS);
            send(stat.set(dimmerLevel[id - dimmerChannelOffset] > 0));
            send(dim.set(dimmerLevel[id - dimmerChannelOffset]));
        }
        else if(id == 0) {
            MyMessage m(id, V_VOLTAGE);
            send(m.set(getVoltage()));
        }
    }
}

void fadeToLevel(byte outputId, byte level)
{
    targetDimValue[outputId] = level;
    fadeRunning[outputId] = true;
}

void toggleOutput(byte outputId) {
    setOutput(outputId, !outputState[outputId]);
    MyMessage stat(outputId + powerChannelOffset, V_STATUS);
    send(stat.set(outputState[outputId]));
}

void setOutput(byte outputId, bool state) {
    digitalWrite(powerOutputs[outputId], state);
    outputState[outputId] = state;
}

unsigned int getVoltage() {
    float volt = (float)analogRead(voltageMeasurePin) * voltsPerADCStep;
    unsigned int mv = (unsigned int)(volt * 1000);
    return mv;
}

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

void setBrightness(byte dimmerId, byte level) { //level: 0-100
    byte val = map(level, 0, 100, 0, 255);
    byte correctedVal = pgm_read_byte(&gamma8[val]);
    analogWrite(dimmerOutputs[dimmerId], correctedVal);
}

void fadeTick() {
    for(byte i = 0; i < sizeof(dimmerOutputs); i++) {
        if(fadeRunning[i]) {
            byte target = targetDimValue[i];
            byte level = dimmerLevel[i];
            if(target != level) {
                short delta = (target > level) ? 1 : -1;
                level += delta;
                setBrightness(i, level);
                dimmerLevel[i] = level;
            }
            else {
                fadeRunning[i] = false;
            }
        }
        
    }
}