// ATTENTION: this is a sample config file. Replace placeholder values with correct ones and rename file to "config.h"


// Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mySensorsGateway-out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mySensorsGateway-in"

// Enable these if your MQTT broker requires usenrame/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// MQTT broker ip address.
#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 123

// The MQTT broker port to to open
#define MY_PORT 1883

// Set WIFI SSID and password
#define MY_ESP8266_SSID "MySSID"
#define MY_ESP8266_PASSWORD "MyVerySecretPassword"