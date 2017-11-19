/**
 * Created by Leandro Späth
 * 
 * This ESP8266 MQTT gateway builds the bridge between the MySensors network and external controllers.
 * Connectivity can be established either using the serial port or MQTT.
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
 * LED purposes:
 * - To use the feature, uncomment any of the MY_DEFAULT_xx_LED_PINs in your sketch
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error
 * 
 * See http://www.mysensors.org/build/esp8266_gateway for wiring instructions.
 * nRF24L01+  ESP8266
 * VCC        VCC
 * CE         GPIO4
 * CSN/CS     GPIO15
 * SCK        GPIO14
 * MISO       GPIO12
 * MOSI       GPIO13
 *******************************
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
// #define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#define MY_GATEWAY_MQTT_CLIENT
#define MY_GATEWAY_ESP8266

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-1"

// sensitive configuration saved in separate file
#include "config.h"

// Enable inclusion mode
//#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

// Set blinking period
//#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Flash leds on rx/tx/err
//#define MY_DEFAULT_ERR_LED_PIN 16  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  16  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED

#define RF433_TRANSMIT_PIN 16
#define RF433_RECEIVE_PIN  5

#include <ESP8266WiFi.h>
#include <MySensors.h>

#include <RCSwitch.h>

RCSwitch rf433 = RCSwitch();



void setup()
{
    // Setup locally attached sensors
    rf433.enableReceive(RF433_RECEIVE_PIN);
    rf433.enableTransmit(RF433_TRANSMIT_PIN);
}

#define C_433_ID 0
#define C_IR_ID  1

void presentation()
{
    // Present locally attached sensors here
    present(C_433_ID, S_CUSTOM, "433MHz sending/receiving of codes");
    present(C_IR_ID, S_IR, "IR sender / receiver NOT YET IMPLEMENTED");
}

void loop()
{
    // Send locally attech sensors data here
    check_rx_433();
}

void check_rx_433() {
    if(rf433.available()) {
        uint32_t rxCode = rf433.getReceivedValue();
        rf433.resetAvailable();
        MyMessage rxMsg(C_433_ID, V_VAR1);
        send(rxMsg.set(rxCode));
    }
}

void tx_433(unsigned long code) {
    rf433.disableReceive(); //disable receive as to not pick up own transmitted signal

    rf433.send(code, 24);

    rf433.enableReceive(RF433_RECEIVE_PIN); //reenable receive
}

void receive(const MyMessage &message)
{
    if(message.getCommand() == C_SET) {
        if(message.sensor == C_433_ID) {
            if(message.type == V_VAR1) {
                unsigned long code = message.getULong();
                tx_433(code);
            }
        }
        
    }
}

