#ifndef CONFIG_H_
#define CONFIG_H_

#define VERSION_NUM "version 1.0"

//WiFi credentials
static const char ssid[] PROGMEM = "";
static const char pwd[] PROGMEM = "";

//auth0
static const char client_id[] PROGMEM = "";
static const char secret[] PROGMEM = "";

//Server
const int httpsPort = 443;
static const char host[] PROGMEM = "sams.science.itf.llu.lv";

//Request strings
static const char acceptPart[] PROGMEM = "\r\nAccept:application/json\r\nContent-Length:";
static const char contentTypePart[] PROGMEM = "\r\nContent-Type:application/json\r\nAuthorization: Bearer ";
static const char connClosePart[] PROGMEM = "\r\nConnection:Close\r\n\r\n";

//----------Enabled sensors-----------------
#define DHT_ENABLED   1
#define SCALE_ENABLED 1
#define DS18_ENABLED  1
#define BAT_ENABLED   0
//------------------------------------------

//----------Sensor related configs----------
//DHT22
#define DHTPIN  12
#define DHTTYPE DHT22

//HX711
#define DOUT  4
#define SCK   5
const float scaleFactor = 103.44f;
const long scaleOffset = 94834;//109910;

//DS18x20
#define DSPIN           14
#define DS_SENSOR_COUNT 5 //some hardcoded maximum limit for 1-wire sensors

//Voltage measurement
const float voltageFactor = 5.4f;
//------------------------------------------

#endif
