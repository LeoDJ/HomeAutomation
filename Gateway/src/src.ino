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

// Enables and select radio type
#define MY_RADIO_RF24
#define MY_RF24_PA_LEVEL RF24_PA_MAX

//#define MY_GATEWAY_SERIAL

#define MY_GATEWAY_MQTT_CLIENT
#define MY_GATEWAY_ESP8266

// On the SHMod24 PCB, the CE pin is on GPIO16
#define MY_RF24_CE_PIN 16

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

#include "ota.h"

void setup()
{
    initOTA();
}


void presentation()
{
    // Present locally attached sensors here
}

void loop()
{
    // Send locally attech sensors data here
    loopOTA();

    if(millis() > 30000 && WiFi.status() != WL_CONNECTED)
        ESP.restart();
}

void receive(const MyMessage &message)
{

}

