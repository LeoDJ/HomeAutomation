// ATTENTION: this is a sample config file. Replace placeholder values with correct ones and rename file to "config.h"

const char* ssid = "xxx";
const char* pass = "xxx";

const char* otaPassword = "xxx"; //leave empty for no password

const char* influxHost = "xxx";                 //IP address or hostname
const int influxPort = 8086;                    //default port
const String database = "xxx";
const String measurement = "xxx";               //measurement to push the data to
const String auth = "dXNlcm5hbWU6cGFzc3dvcmQ="; //simple base64 encoded string username:password, leave empty for no authentication

//leave empty for automatic naming based on hostname, node id and child id
const String sensorNames[] = {
    "Balcony",
    "Room 1"
};