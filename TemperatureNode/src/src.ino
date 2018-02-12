/**
 * Created by Leandro Späth
 * 
 * This sketch implements a MySensors node to keep track of your energy usage.
 * The sensor attaches to a meter with a turning wheel or blinking light.
 * In this case the sketch is used to read a Ferraris wheel meter
 * It will report the current power usage in watts.
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
 * 1: <reserved>
 * 2: temperature sensor debug info
 * NOTE: the value gets checked as binary, so you can combine as you like
 *       e.g.: 3 shows debug messages from 1 and 2
 */
#define USER_DEBUG 2

#define MY_NODE_ID 6

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
#include <DallasTemperature.h>
#include <OneWire.h>


#define ONE_WIRE_PIN    6      // OneWire Bus pin for attaching the DS18B20 temperature Sensor
#define TEMP_INTERVAL   300000 // Time between temperature updates
#define TEMP_AVG_COUNT  10     // Number of readings to take for each data point


#define TEMP_SENSOR_COUNT 2    // amount of plugged in temperature sensors

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature tempSensor(&oneWire);
#define TEMP_CONVERSION_WAIT 750         // wait time in ms before conversion is complete
//MyMessage tempMsg(C_TEMP_ID, V_TEMP);
float lastTemperature[TEMP_SENSOR_COUNT], tempSum[TEMP_SENSOR_COUNT];
byte tempMeasureCount = 0;
unsigned long lastTempRead = 0, lastTempSend = 0;

void before()
{
  // Startup up the OneWire library
  tempSensor.begin();
}

void setup() {
    // put your setup code here, to run once:

    // requestTemperatures() will not block current thread
    tempSensor.setWaitForConversion(false);
    tempSensor.requestTemperatures(); // initial temperature request
    lastTempRead = millis(); // make sure function waits enough
}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("TemperatureNode1", "0.0.1");
    
    for(byte i = 1; i <= TEMP_SENSOR_COUNT; i++) {
        present(i, S_TEMP);
    }
}

unsigned long now;

void loop() {

    temperatureTick();
   
}

void temperatureTick() {
    now = millis();
     //start temperature measuring every interval
    if(now > lastTempSend + TEMP_INTERVAL) {
        lastTempSend = now;
        lastTempRead = now;
        tempMeasureCount = 0;
        tempSensor.requestTemperatures();
    }

    //call every measurement tick
    if(tempMeasureCount < TEMP_AVG_COUNT && now > lastTempRead + TEMP_CONVERSION_WAIT) {
        lastTempRead = now;

        //iterate through all sensors
        for(byte i = 0; i < TEMP_SENSOR_COUNT; i++) {
            float temp = tempSensor.getTempCByIndex(i);
            if(temp > -127 && temp < 85) { // filter out false reading
                tempSum[i] += temp;
                //when at last sensor, start next tick
                if(i == TEMP_SENSOR_COUNT - 1) {
                    tempMeasureCount++;
                    tempSensor.requestTemperatures();
                }
            }
            #if USER_DEBUG & 2
            else {
                Serial.print("no valid reading: ");
            }
                Serial.print(now);              Serial.print(" ");
                Serial.print(i);                Serial.print(" ");
                Serial.print(temp);             Serial.print(" ");
                Serial.print(tempMeasureCount); Serial.print(" ");
                Serial.println();
            #endif
        }

        //check if all measurements are taken
        if(tempMeasureCount >= TEMP_AVG_COUNT) {
            //iterate through all sensors
            for(byte i = 0; i < TEMP_SENSOR_COUNT; i++) {
                float temp = tempSum[i] / tempMeasureCount;
                MyMessage tempMsg(i, V_TEMP);
                send(tempMsg.set(temp, 2)); //send temperature with 2 decimal places
                tempSum[i] = 0;

                #if USER_DEBUG & 2
                    Serial.print(F("send temp from sensor "));
                    Serial.print(i);
                    Serial.print(": ");
                    Serial.print(temp);
                    Serial.println();
                #endif
            }
        }
    }
}