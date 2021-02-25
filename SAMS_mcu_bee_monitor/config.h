#ifndef CONFIG_H_
#define CONFIG_H_

#define VERSION_NUM "version 1.2"

//Sleep time in seconds
#define SLEEPTIME 60

//WiFi credentials
static const char ssid[] PROGMEM = "";
static const char pwd[] PROGMEM = "";

//Device credentials
static const char client_id[] PROGMEM = "";
static const char secret[] PROGMEM = "";

//Server
const int httpsPort = 443;
static const char host[] PROGMEM = "sams.science.itf.llu.lv";

//Request strings
static const char grantType[] PROGMEM = "client_credentials";
static const char audience[] PROGMEM = "sams-dwh-web-api";
static const char apiHost[] PROGMEM = "POST /api/data HTTP/1.1\r\nHost:";
static const char apiLogs[] PROGMEM = "POST /api/logs HTTP/1.1\r\nHost:";
static const char apiToken[] PROGMEM = "POST /api/token HTTP/1.1\r\nHost:";
static const char apiConfig[] PROGMEM = "GET /api/configs/device HTTP/1.1\r\nHost:";
static const char acceptPart[] PROGMEM = "\r\nAccept:application/json\r\nContent-Length:";
static const char contentTypePart[] PROGMEM = "\r\nContent-Type:application/json";
static const char bearerPart[] PROGMEM = "\r\nAuthorization: Bearer ";
static const char connClosePart[] PROGMEM = "\r\nConnection:Close\r\n\r\n";

//----------Enabled sensors-----------------
#define DHT_ENABLED   1
#define SCALE_ENABLED 1
#define DS18_ENABLED  1
#define BAT_ENABLED   1
//------------------------------------------

//----------Sensor related configs----------
//DHT22
#define DHTPIN  12
#define DHTTYPE DHT22

//HX711
#define DOUT  4
#define SCK   5
const float scaleFactor = ;
const long scaleOffset = ;

//DS18x20
#define DSPIN           14
#define DS_SENSOR_COUNT 5 //some hardcoded maximum limit for 1-wire sensors

//Voltage measurement
const float voltageFactor = 5.4f;
//------------------------------------------

#endif
