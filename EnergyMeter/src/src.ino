/**
 * Created by Leandro Späth
 * 
 * This sketch implements a MySensors node to keep track of your energy usage
 * The sensor attaches to a meter with a turning wheel or blinking light
 * It will report the current power usage in watts
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

#define PULSE_FACTOR 375       // Number of rotations/blinks per kWh of your meter
#define C_POWER_ID 0

double ppwh = ((double)PULSE_FACTOR/1000);  // pulses per watt hour

void setup()
{

}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Energy Meter", "0.1.0");
    
    // Register this device as power sensor
    present(C_POWER_ID, S_POWER);
}

void loop()
{
	unsigned long now = millis();
}

void receive(const MyMessage &message)
{

}