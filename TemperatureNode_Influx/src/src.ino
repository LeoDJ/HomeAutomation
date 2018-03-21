/**
 * Created by Leandro Späth
 * 
 * This sketch reads the temperature of one or multiple connected DS18B20 temperature sensors.
 * It then pushes the acquired data to an influxDB instance.
 * The data is inserted like MySensors over Home Assistant would do, to allow a seamless integration
 * 
 * All sensitive configuration is done in the config.h file (rename config_sample.h)
 *  
 * future TODO: keep track of known sensors in EEPROM or something like that
 */


/**
 * sets the debug output
 * 0: nothing
 * 1: WiFi debug
 * 2: temperature sensor debug info
 * 4: HTTP request debug
 * NOTE: the value gets checked as binary, so you can combine as you like
 *       e.g.: 3 shows debug messages from 1 and 2
 */
#define USER_DEBUG 3

#define OTA_ENABLED

#define ONE_WIRE_PIN      0      // OneWire Bus pin for attaching the DS18B20 temperature Sensor
#define TEMP_SENSOR_COUNT 2      // amount of plugged in temperature sensors
#define TEMP_INTERVAL     300000 // Time between temperature updates
#define TEMP_AVG_COUNT    10     // Number of readings to take for each data point

#define HOSTNAME "TemperatureNodeInflux1"
#define NODE_ID  "6"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#if defined(OTA_ENABLED)
    #include <ESP8266mDNS.h>
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif

#include "config.h"



OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature tempSensor(&oneWire);
#define TEMP_CONVERSION_WAIT 750         // wait time in ms before conversion is complete
float lastTemperature[TEMP_SENSOR_COUNT], tempSum[TEMP_SENSOR_COUNT];
byte tempMeasureCount = 0, sensorMeasureCount[TEMP_SENSOR_COUNT];
DeviceAddress sensorAddresses[TEMP_SENSOR_COUNT];
unsigned long lastTempRead = 0, lastTempSend = 0;

WiFiClient client;

void setup() {
    delay(500);
    Serial.begin(115200);
    Serial.println(" >> TemperatureNode_Influx << ");

    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        timeout++;
        if(timeout==30) {
            ESP.restart();
        }
    }

    #if USER_DEBUG & 1
        Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    #endif


    #if defined(OTA_ENABLED)
        ArduinoOTA.setHostname(HOSTNAME);
        if(sizeof(otaPassword) > 1)
            ArduinoOTA.setPassword(otaPassword);
        ArduinoOTA.onStart([]() {
            Serial.println("OTA Start");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\nEnd OTA");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });
        ArduinoOTA.begin();
    #endif


    tempSensor.begin();
    indexSensors();
    tempSensor.setWaitForConversion(false);
    tempSensor.requestTemperatures(); // initial temperature request
    lastTempRead = millis(); // make sure function waits enough
}




void loop() {
    temperatureTick();
    ArduinoOTA.handle();
}


void indexSensors() {
    for(byte i = 0; i < TEMP_SENSOR_COUNT; i++) {
        DeviceAddress devAddr;
        if(tempSensor.getAddress(devAddr, i)) {
            memcpy(sensorAddresses[i], devAddr, sizeof(sensorAddresses[i]));
            #if USER_DEBUG & 2
                printAddress(sensorAddresses[i]);
                Serial.println();
            #endif
        }
    }
}
#if USER_DEBUG & 2
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}
#endif


void temperatureTick() {
    unsigned long now = millis();
     //start temperature measuring every interval
    if(now > lastTempSend + TEMP_INTERVAL) {
        lastTempSend = now;
        lastTempRead = now;
        tempMeasureCount = 0;
        for(byte i = 0; i < TEMP_SENSOR_COUNT; i++) {
            tempSum[i] = 0;
            sensorMeasureCount[i] = 0;
        }
        tempSensor.requestTemperatures();
    }

    //call every measurement tick
    if(tempMeasureCount < TEMP_AVG_COUNT && now > lastTempRead + TEMP_CONVERSION_WAIT) {
        lastTempRead = now;

        //iterate through all sensors
        bool someSensorSuccessful = false;
        for(byte i = 0; i < TEMP_SENSOR_COUNT; i++) {
            float temp = tempSensor.getTempC(sensorAddresses[i]); //request sensors in order of detection upon startup
            if(temp > -127 && temp < 85) { // filter out false reading
                tempSum[i] += temp;
                sensorMeasureCount[i]++;
                someSensorSuccessful = true;
                //when at last sensor, start next tick
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

            if(i == TEMP_SENSOR_COUNT - 1 && someSensorSuccessful) {
                tempMeasureCount++;
            }
        }
        tempSensor.requestTemperatures();

        //check if all measurements are taken
        if(tempMeasureCount >= TEMP_AVG_COUNT) {
            sendTemperatureValues();
            Serial.println(String(now) + "  Sensor values sent successfully");
        }
    }
}


void sendTemperatureValues() {
    String payload = "";
    //build line protocol insert statement
    //imitate mySensors data written through home assistant into influx
    for(byte i = 0; i < TEMP_SENSOR_COUNT; i++) {
        if(sensorMeasureCount[i] > 0) {
            String childId = String(i+1);
            String nodeId = NODE_ID;
            String temp = String(tempSum[i] / sensorMeasureCount[i], 2); //convert temperature to string with 2 decimal places
            payload += measurement+ ",";
            payload += "child_id=" + childId + ",";
            payload += "node_id=" + nodeId + ",";
            payload += "entity_id=temperature_node_" + nodeId + "_" + childId + ",";
            payload += "friendly_name=Temperature\\ Node\\ " + nodeId + "\\ " + childId + ",";
            payload += "domain=sensor "; //end tags, begin value
            payload += "V_TEMP=" + temp + ",";
            payload += "value=" + temp + ",";
            payload += "device_str=\"direct_influx\"";
            payload += "\n";
        }
    }
    int payloadLength = payload.length();

    #if USER_DEBUG & 4
        Serial.println("INSERTING DATA:");
        Serial.println(payload);
        Serial.println();
        Serial.print("connecting to ");
        Serial.println(influxHost);
    #endif
    if (!client.connect(influxHost, influxPort)) {
        Serial.println("ERROR: HTTP connection to influxDB failed");
        return;
    }

    String httpRequest = String("POST /write?db=") + database + " HTTP/1.1\r\n" + 
        "Host: " + influxHost + "\r\n" + 
        ((auth.length() > 0) ? "Authorization: Basic " + auth + "\r\n" : "") +
        "Connection: close\r\n" + 
        "Content-Type: application/x-www-form-urlencoded\r\n" +
        "Accept: */*\r\n" +
        "User-Agent: ESP8266 WiFiClient\r\n" +
        "Connection: close\r\n" +
        "Content-Length:" + payloadLength + "\r\n\r\n" + payload + "\r\n";

    #if USER_DEBUG & 4
        Serial.println();
        Serial.println("HTTP REQUEST:");
        Serial.println(httpRequest);
    #endif

    client.print(httpRequest);

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        #if USER_DEBUG & 4
            Serial.println(line);
        #endif
        if(line.indexOf("204 No Content") != -1) {
            #if USER_DEBUG & 4
                Serial.println("influxdb successfull!");
            #endif
        }
        if (line == "\r") {
            #if USER_DEBUG & 4
                Serial.println("headers received");
                Serial.println(" ");
            #endif
            break;
        }
    }
}
