/**
 * Created by Leandro Späth
 * 
 * This sketch is used to control multiple 12V outputs including dimming of my 12V distribution box
 * 
 * All sensitive configuration is done in the config.h file (rename config_sample.h)
 * 
 * More information can be found in the blog article: <link coming soon>
 * 
 *******************************
 * Uses the MySensors Arduino library
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 *******************************
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_NODE_ID 1

// Enable and select radio type attached
#define MY_RADIO_NRF24

#define MY_RF24_CE_PIN 7
#define MY_RF24_CS_PIN 8

// Enable repeater
#define MY_REPEATER_FEATURE

#define MY_TRANSPORT_WAIT_READY_MS 5000
#define MY_TRANSPORT_SANITY_CHECK

// sensitive configuration saved in separate file
#include "config.h"

#include <MySensors.h>

#define USER_DEBUG false

#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
const byte dimmerButtonDivisor = 20; //do one fade step every x ms
const int  dimmerDimDelay = 300; //time in ms to wait before dimming sets in, before that it simply switches

const byte voltageMeasurePin = A7;
const float voltsPerADCStep = 0.016673; // using 1.1V reference and a resistor ratio of 15:1
#define VOLTAGE_PUBLISH_INTERVAL 60000   // interval in ms, by which the voltage reading is publishted to the mySensors network

const byte dimmerOutputs[] = {9}; //has to be transistor/mosfet
const byte powerOutputs[] = {3, 6, 5}; //pins with relays/mosfets connected to them to switch outputs
const byte buttons[] = {A3, A0, A2, A1}; //maps to all dimmers then to all outputs in ascending order

#define CID_VOLTAGE 0  // child id of voltage sensor
const byte dimmerChannelOffset = 4; //move digital pins behind in the channel mapping (for expandability)
const byte powerChannelOffset = 8; 

const byte dimCount = sizeof(dimmerOutputs), outCount = sizeof(powerOutputs);

byte dimmerLevel[dimCount], targetDimValue[dimCount], lastDimLevel[dimCount];
char dimmerDelta[dimCount] = {0};
bool fadeRunning[dimCount];
bool outputState[outCount];

#define VOLTAGE_READING_COUNT    200
#define VOLTAGE_MEASURE_INTERVAL 10   // take a reading every 10ms
float voltageAvg = 0;

byte debounceTime = 40;

void setup()
{
    //set analog reference to the internal 1.1V reference for better voltage measuring accuracy
    analogReference(INTERNAL);

    
    MyMessage stat(0, V_STATUS); 
    MyMessage dim(0, V_DIMMER); 

    for(byte i = 0; i < sizeof(buttons); i++) {
        pinMode(buttons[i], INPUT_PULLUP);
    }
    for(byte i = 0; i < outCount; i++) { //sizeof reports size in bytes
        byte cid = i + powerChannelOffset;
        //bool state = request(i + powerChannelOffset, V_STATUS); //currently does not work
        bool state = loadState(cid);
        send(stat.setSensor(cid).set(state)); //workaround for sensor being recognized in home assistant
        pinMode(powerOutputs[i], OUTPUT);
        setOutput(i, state);
    }
    for(byte i = 0; i < dimCount; i++) { //sizeof reports size in bytes
        byte cid = i + dimmerChannelOffset;
        //byte level = request(i + dimmerChannelOffset, V_PERCENTAGE); //currently does not work
        byte level = loadState(cid);  
        send(dim.setSensor(cid).set(level)); //workaround for sensor being recognized in home assistant
        send(stat.setSensor(cid).set(level > 0));
        if(level > 0) {
            lastDimLevel[i] = 0;
            targetDimValue[i] = level;
            dimmerDelta[i] = 1;
        }
        else {
            lastDimLevel[i] = 100;
        }
        pinMode(dimmerOutputs[i], OUTPUT);
        fadeToLevel(i, level);
    }

    //initialize voltage averaging
    voltageAvg = (float)analogRead(voltageMeasurePin) * voltsPerADCStep; 
}

void presentation()
{
    // Register the LED Dimmable Light with the gateway
    present(CID_VOLTAGE, S_MULTIMETER);
    for(byte i = 0; i < dimCount; i++) { //sizeof reports size in bytes
        present(i + dimmerChannelOffset, S_DIMMER);
        request(i + dimmerChannelOffset, S_DIMMER, MY_NODE_ID); //workaround for sensor being recognized in home assistant
    }
    for(byte i = 0; i < outCount; i++) { //sizeof reports size in bytes
        present(i + powerChannelOffset, S_BINARY);
        request(i + powerChannelOffset, S_BINARY, MY_NODE_ID); //workaround for sensor being recognized in home assistant
    }
    
	sendSketchInfo("12V Distribution Box 01", "1.0.0");
}


unsigned long lastFadeTick = 0, lastVoltageMeasure = 0, lastVoltagePublish = 0;

void loop()
{

    if(millis() - lastFadeTick >= FADE_DELAY) { //run fading, if necesary
        lastFadeTick = millis();
        fadeTick();
    }

    // take voltage measurements for more accurate voltage reading
    if(millis() - lastVoltageMeasure >= VOLTAGE_MEASURE_INTERVAL) { 
        lastVoltageMeasure = millis();
        //build a rolling average
        voltageAvg -= voltageAvg / VOLTAGE_READING_COUNT;
        float v = (float)analogRead(voltageMeasurePin) * voltsPerADCStep;
        voltageAvg += v / VOLTAGE_READING_COUNT;
    }

    if(millis() - lastVoltagePublish >= VOLTAGE_PUBLISH_INTERVAL) {
        lastVoltagePublish = millis();
        MyMessage m(CID_VOLTAGE, V_VOLTAGE);
        send(m.set(voltageAvg, 3));
    }

    handleButtons();
}

unsigned long lastBtnPress = 0;
unsigned long buttonPressedAt[sizeof(buttons)] = {0};

byte getTimeDiffDimmerValue(byte id) {
    byte val = constrain((int)dimmerLevel[id] + (int)(dimmerDelta[id] * ((millis() - buttonPressedAt[id] - dimmerDimDelay) / dimmerButtonDivisor)), 0, 100); //hopefully doesn't overflow
    //Serial.println("dimlev: " + String(dimmerLevel[id]) + " | dimDelt: " + String((int)dimmerDelta[id]) + " | calcDetail: " + String((dimmerDelta[id] * (millis() - buttonPressedAt[id]) / dimmerButtonDivisor)) + " | val: " + String(val));
    return val;
}

void handleButtons() {
    for(byte i = 0; i < sizeof(buttons); i++) {
        bool state = digitalRead(buttons[i]);
        bool isDigital = i >= dimCount;
        if(!state && buttonPressedAt[i] == 0) { //button rising edge
            buttonPressedAt[i] = millis(); //save when button was pressed
            #if USER_DEBUG
                Serial.println(String(millis()) + " Button B" + String(i) + " rising edge detected");
            #endif
        }
        else if(!state && buttonPressedAt[i] != 0) { //button is being held
            if(!isDigital) { //handling for dimmer buttons | TODO: can be placed in if clause above
                if(millis() - buttonPressedAt[i] > dimmerDimDelay) { //if in dimming range of button press
                    if(dimmerLevel[i] == 0) //set dimming direction, if at end of range
                        dimmerDelta[i] = 1;
                    else if(dimmerLevel[i] == 100) 
                        dimmerDelta[i] = -1;
                    setBrightness(i, getTimeDiffDimmerValue(i));
                    //Serial.println(String(millis()) + " Button B" + String(i) + " currently dimming... " + String(getTimeDiffDimmerValue(i)));
                }
            }
        }
        else if(state && buttonPressedAt[i] != 0) { //if button released
            #if USER_DEBUG
                Serial.println(String(millis()) + " Button B" + String(i) + " released. Was pressed at " + String(buttonPressedAt[i]));
            #endif
            if(!isDigital) {
                byte newLevel;
                if(millis() - buttonPressedAt[i] > dimmerDimDelay) { //if it had been dimming 
                    dimmerLevel[i] = getTimeDiffDimmerValue(i);
                    dimmerDelta[i] = -dimmerDelta[i];
                    if(dimmerLevel[i] > 0) //workaround for the button push to correctly turn off LED
                        lastDimLevel[i] = 0;
                    newLevel = dimmerLevel[i];
                    targetDimValue[i] = newLevel;
                    saveState(i + dimmerChannelOffset, newLevel); //save state to EEPROM
                }
                else { //if dimmer button was only shortly pushed
                    fadeToLevel(i, lastDimLevel[i]);
                    newLevel = lastDimLevel[i];
                    if(lastDimLevel[i] == 0)
                        lastDimLevel[i] = dimmerLevel[i];
                    else 
                        lastDimLevel[i] = 0;
                    //Serial.println(String(millis()) + " Button B" + String(i) + " released. New dimLev: " + String(dimmerLevel[i]) + " | lastLev: " + String(lastDimLevel[i]));
                }
                buttonPressedAt[i] = 0;
                MyMessage dim(i + dimmerChannelOffset, V_DIMMER); //hopefully won't cause an overflow
                MyMessage stat(i + dimmerChannelOffset, V_STATUS);
                send(stat.set(newLevel > 0));
                send(dim.set(newLevel));
                
            }
            else if(isDigital && millis() - buttonPressedAt[i] >= debounceTime) {
                toggleOutput(i - dimCount);
                #if USER_DEBUG
                    Serial.println(String(millis()) + " Button B" + String(i) + " toggled digital output " + String(buttonPressedAt[i]));
                #endif
                buttonPressedAt[i] = 0;
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
                if(status) { //correct on/off switching with level saving
                    if(lastDimLevel[outputId] != 0) {
                        fadeToLevel(outputId, lastDimLevel[outputId]);
                        lastDimLevel[outputId] = 0;
                    }
                    else {
                        fadeToLevel(outputId, 100);
                        lastDimLevel[outputId] = 0;
                    }
                }
                else {
                    lastDimLevel[outputId] = targetDimValue[outputId];
                    fadeToLevel(outputId, 0);
                }
                MyMessage dim(message.sensor, V_DIMMER);
                send(dim.set(targetDimValue[outputId]));
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
            send(stat.set(targetDimValue[id - dimmerChannelOffset] > 0));
            send(dim.set(targetDimValue[id - dimmerChannelOffset]));
        }
        else if(id == CID_VOLTAGE) {
            MyMessage m(id, V_VOLTAGE);
            send(m.set(voltageAvg, 3));
        }
    }
}

void fadeToLevel(byte outputId, byte level)
{
    targetDimValue[outputId] = level;
    fadeRunning[outputId] = true;
    saveState(outputId + dimmerChannelOffset, level); //save state to EEPROM
}

void toggleOutput(byte outputId) {
    setOutput(outputId, !outputState[outputId]);
    MyMessage stat(outputId + powerChannelOffset, V_STATUS);
    send(stat.set(outputState[outputId]));
}

void setOutput(byte outputId, bool state) {
    digitalWrite(powerOutputs[outputId], state);
    outputState[outputId] = state;
    saveState(outputId + powerChannelOffset, state); //save state to EEPROM
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
    for(byte i = 0; i < dimCount; i++) {
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