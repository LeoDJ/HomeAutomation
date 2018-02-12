// ATTENTION: this is a sample config file. Replace placeholder values with correct ones and rename file to "config.h"


// Set NRF pipe ID
#define MY_RF24_BASE_RADIO_ID 0x00,0xFC,0xE1,0xA8,0xA8


// Set WIFI SSID and password
#define MY_ESP8266_SSID "MySSID"
#define MY_ESP8266_PASSWORD "MyVerySecretPassword"

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
#define MY_ESP8266_HOSTNAME "mqtt-sensor-gateway"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,178,87

// If using static ip you can define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,178,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0


// Set this node's MQTT subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mySensorsGateway-out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mySensorsGateway-in"

// MQTT broker ip address.
#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 123

// The MQTT broker port to to open
#define MY_PORT 1883

// Enable these if your MQTT broker requires usenrame/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-mqtt-gateway"