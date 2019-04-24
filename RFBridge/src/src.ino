/**
 * Created by Leandro Späth
 *
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

/**
 * sets the debug output
 * 0: nothing
 * NOTE: the value gets checked as binary, so you can combine as you like
 */
#define USER_DEBUG 0

#define MY_NODE_ID 7
// #define MY_GATEWAY_SERIAL //for development purposes without gateway

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

#include "globals.h"
#include "ir.h"
#include "rf.h"



void setup() {
    Serial.begin(115200);
    irSetup();
    rfSetup();
}

void presentation() {
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("RFBridge", "1.0.0");
    present(CID_IR, S_IR, "IR TX/RX");
    present(CID_RF, S_IR, "433MHz TX/RX");
}

unsigned long now;

uint32_t hassInitDelay = 5000;
void hassInit() {
    // send initial message for all variables (needed for Home Assistant auto detection)
    #define IR_NULL "00,00000000,00,0000,00"
    MyMessage msgIr(CID_IR, V_IR_SEND);
    send(msgIr.set(IR_NULL));
    msgIr.setType(V_IR_RECEIVE);
    send(msgIr.set(IR_NULL));

    #define RF_NULL "00,00000000,00,0000"
    MyMessage msgRf(CID_RF, V_IR_SEND);
    send(msgRf.set(RF_NULL));
    msgRf.setType(V_IR_RECEIVE);
    send(msgRf.set(RF_NULL));
}

void loop() {
    now = millis();

    if(hassInitDelay > 0 && millis() > hassInitDelay) {
        hassInit();
        hassInitDelay = 0;
    }

    irLoop();
    rfLoop();
}

void receive(const MyMessage &message) {
    irReceive(message);
    rfReceive(message);
}

