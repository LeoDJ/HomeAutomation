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

//#define MY_NODE_ID 7
#define MY_GATEWAY_SERIAL //for development purposes without gateway
//Beamer on: 0;1;1;0;32;03,10C8E11E,20,0,01

// Enable and select radio type attached
#define MY_RADIO_NRF24

#define MY_RF24_CE_PIN 7
#define MY_RF24_CS_PIN 8

// Enable repeater
//#define MY_REPEATER_FEATURE

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
    present(CID_IR, S_IR);
}

unsigned long now;

void loop() {
    now = millis();

    irLoop();
    rfLoop();
}

void receive(const MyMessage &message) {
    irReceive(message);
    rfReceive(message);
}

