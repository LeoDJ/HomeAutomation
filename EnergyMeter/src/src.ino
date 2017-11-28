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
 * TODO: temperature sensor
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
//#define MY_DEBUG

/**
 * sets the debug output
 * 0: nothing
 * 1: show the raw sensor values (useful for calibrating)
 * 2: show pulse timing information
 * NOTE: the value gets checked as binary, so you can combine as you like
 *       e.g.: 3 shows raw sensor and timing (1+2)
 */
#define USER_DEBUG 2

#define MY_NODE_ID 5

// Enable and select radio type attached
#define MY_RADIO_NRF24

//#define MY_RF24_CE_PIN 7
//#define MY_RF24_CS_PIN 8

// Enable repeater
//#define MY_REPEATER_FEATURE

#define MY_TRANSPORT_WAIT_READY_MS 5000
#define MY_TRANSPORT_SANITY_CHECK

// sensitive configuration saved in separate file
#include "config.h"

//#include <MySensors.h>
#include <Arduino.h>

#define PULSE_FACTOR    375   // Number of rotations/blinks per kWh of your meter
#define LED_PIN         5     // pin with a LED attached to display energy unit consumed

#define CNY70_LED_PIN   4     // IR LED pin of CNY70 sensor
#define CNY70_SENS_PIN  A0    // Phototransistor pin of CNY70 sensor
#define TRIGGER_THRESH  5     // analogRead threshold before a meter pulse is triggered
#define TRIGGER_EDGE FALLING  // whether to trigger on a falling or a rising edge

#define SEND_FREQUENCY  15000 // maximum frequency of status updates
#define MAX_WATT        30000 // limit maximum reported wattage to catch astray readings

// averaging configuration, better don't touch
#define MEASURE_INTERVAL 5    // ms between measurements
#define ROLL_AVG_SAMPLES 20   // how many measurements to take for sensor smoothing (tuned to the measure interval)
#define EWMA_AVG_SAMPLES 500  // how much weight to give the long term average      (tuned to the measure interval)
// NOTE  the rolling average samples occupy quite a bit of ram (2 bytes for each ROLL_AVG_SAMPLE)
//  TO   if the sketch does not compile in the future, change the interval and sample count 
// SELF: accordingly (e.g. measure interval *2, sample counts /2)


#define C_POWER_ID 1

/*MyMessage wattMsg(C_POWER_ID, V_WATT);
MyMessage kwhMsg(C_POWER_ID,V_KWH);
MyMessage pcMsg(C_POWER_ID,V_VAR1);*/

double ppwh = (double)PULSE_FACTOR / 1000;  // pulses per watt hour

bool pulseTriggered = 0;
unsigned long lastTriggered = 0;
unsigned long watts, lastWatts = 0;
unsigned long pulseCount = 0, lastPulseCount = 0;
bool pcReceived = false;

void setup()
{
    Serial.begin(115200);
    // set LEDs as outputs
    pinMode(LED_PIN, OUTPUT);
    pinMode(CNY70_LED_PIN, OUTPUT);
}
/*
void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Energy Meter", "0.1.0");
    
    // Register this device as power sensor
    present(C_POWER_ID, S_POWER);
}*/

unsigned long lastMeasure = 0, lastSend = 0;

void loop()
{
    //Serial.println(millis() - now); // print loop time
    unsigned long now = millis();

    if(now - lastMeasure >= MEASURE_INTERVAL) {
        lastMeasure = now;
        measure();
    }

    if(now - lastSend >= SEND_FREQUENCY) {
        lastSend = now;
        if(watts < MAX_WATT && watts != lastWatts) {
            lastWatts = watts;
            ///send(wattMsg.set(watts));
        }
        if(pcReceived && pulseCount != lastPulseCount) {
            lastPulseCount = pulseCount;
            float kWh = (float)pulseCount / (float)PULSE_FACTOR;
            ///send(kwhMsg.set(kWh, 3));
            ///send(pcMsg.set(pulseCount));
        }
        else if(!pcReceived) {
            // no count received, request again
            ///request(CHILD_ID, V_VAR1);
        }
    }
}

/*
void receive(const MyMessage &message)
{
    if (message.type==V_VAR1) {
		pulseCount = lastPulseCount = message.getLong();
		//Serial.print("Received last pulse count from gw:");
		//Serial.println(pulseCount);
		pcReceived = true;
	}
}*/

/**
 * This function takes the readings from the CNY70 sensor.
 * Then they'll get processed by the averaging function.
 */

void measure() {
    int val_noLed = analogRead(CNY70_SENS_PIN); // read sensor value with IR LED off
    digitalWrite(CNY70_LED_PIN, HIGH);          // turn on IR LED
    delayMicroseconds(200);                     // wait a bit for sensor to saturate
    int val_led = analogRead(CNY70_SENS_PIN);   // read sensor value with IR LED on
    digitalWrite(CNY70_LED_PIN, LOW);           // turn off IR LED
    calcAverage(val_noLed - val_led);           // filter out external light and calculate the averages
}

/**
 * This function builds average values from the readings to detect edges in the signal.
 * The first average is an EWMA that does not need much memory but is not so fast to respond.
 * It is used to build a baseline reading of the sensor, when no red marking is detected.
 * 
 * The second one is a simple moving average that uses a few bytes of RAM to smooth the sensor input 
 * but still be fast enough to detect a quick brightness change
 */

float ewma_avg = -1, roll_avg;
uint16_t roll_n = 0;
int16_t roll_vals[ROLL_AVG_SAMPLES];

void calcAverage(int val)
{
    // save current value in rolling average storage
    roll_vals[roll_n] = val;
    roll_n++;
    if(roll_n >= ROLL_AVG_SAMPLES)
        roll_n = 0;
    // calculate new rolling average value
    roll_avg = 0;
    for(byte i = 0; i < ROLL_AVG_SAMPLES; i++) {
        roll_avg += roll_vals[i];
    }
    roll_avg /= ROLL_AVG_SAMPLES;

    //EWMA
    if(ewma_avg < 0) //ewma not initialized yet, hopefully not too resource intensive
      ewma_avg = val;
    ewma_avg -= ewma_avg / EWMA_AVG_SAMPLES;
    ewma_avg += (float)val / EWMA_AVG_SAMPLES;

    #if USER_DEBUG & 1
        Serial.println(String(val)+" "+String(roll_avg)+" "+String(ewma_avg)+" "+String(pulseTriggered?ewma_avg:ewma_avg-TRIGGER_THRESH));
    #endif

    float diff = (TRIGGER_EDGE == FALLING) ? ewma_avg - roll_avg : roll_avg - ewma_avg;

    if(!pulseTriggered && diff >= TRIGGER_THRESH) {
        pulseTriggered = true;
        trigger();
    }
    else if(pulseTriggered && diff < (TRIGGER_THRESH/2)) { // implement a little bit of hysteresis
        pulseTriggered = false;
    }
}

void trigger() {
    if(lastTriggered == 0) { // filter out first false trigger after startup
        lastTriggered = micros();
        return;
    }

    unsigned long trig = micros();
    digitalWrite(LED_PIN, HIGH); 

    pulseCount++;
    // calculate watts based on time between pulses
    double interval = trig - lastTriggered;   
    watts = 3600000000.0 / (interval * ppwh);

    
    #if USER_DEBUG & 2
        Serial.print(trig);             Serial.print(" ");
        Serial.print(lastTriggered);    Serial.print(" ");
        Serial.print(interval);         Serial.print(" ");
        Serial.print(watts);            Serial.print(" ");
        Serial.println();
    #endif

    lastTriggered = trig;
    delayMicroseconds(200);       // TODO: get rid of this delay
    digitalWrite(LED_PIN, LOW);
}