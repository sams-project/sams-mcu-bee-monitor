#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <RTCVars.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "ds18_20.h"
#include "HX711.h"
#include "config.h"

#define TOKENLENGTH 350
//Sensor count without 1-wire
#define INDIVIDUALSENSORS 4 //DHT22(temp & hum), HX711(load cell), battery voltage

RTCVars rtcState; //RTC state object

//Sensor definition
DHT dht(DHTPIN, DHTTYPE);
HX711 hxScale;
DS18OneWire ds(DSPIN, DS_SENSOR_COUNT);
struct DsSensor dsSensors[DS_SENSOR_COUNT];

struct SAMSSensor {
  char typeName[25];
  float value;
  int8_t canSend;
} sensors[INDIVIDUALSENSORS + DS_SENSOR_COUNT];

//------------DHT22------------------
void readDHT22(struct SAMSSensor* dhtTemp, struct SAMSSensor* dhtHum) {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int8_t isReady = 1;

  //Subtract 5 chars from client_id
  char tempName[6] = {0};

  memcpy_P(tempName, client_id, sizeof(tempName) - 1);

  snprintf(dhtTemp->typeName, strlen("dht22-temp-") + sizeof(tempName), "%s%s", "dht22-temp-", tempName);
  snprintf(dhtHum->typeName, strlen("dht22-hum-") + sizeof(tempName), "%s%s", "dht22-hum-", tempName);

  if (isnan(t) || isnan(h)) {
    isReady = 0;
    t = 0.0f;
    h = 0.0f;
  }

  dhtTemp->value = t;
  dhtTemp->canSend = isReady;
  dhtHum->value = h;
  dhtHum->canSend = isReady;
}
//-----------------------------------

//------------HX711------------------
int8_t comp_fn(long *frst, long *scnd) {
  if (*frst < *scnd) {
    return 1;
  } else if (*frst > *scnd) {
    return -1;
  } else {
    return 0;
  }
}

void readScaleHX711(struct SAMSSensor* loadCell) {
  float weight = 0;
  int8_t isReady = 0;
  hxScale.power_up();
  if (hxScale.wait_ready_retry(10, 50)) {
    int8_t times = 10;
    float scaleReadings[times];
    hxScale.set_offset(scaleOffset);
    hxScale.set_scale(scaleFactor);
    //Throw away first values
    hxScale.get_units(5);
    //Read n times and store in array
    for (int8_t i = 0; i < times; i++) {
      scaleReadings[i] = hxScale.get_units(1);
    }
    hxScale.power_down();
    //Sort recorded values
    qsort(scaleReadings, times, sizeof(float), (int(*)(const void*, const void*))comp_fn);
    int8_t middle = times / 2;
    //Get median value
    if (times % 2 == 0) {
      weight = (scaleReadings[middle] + scaleReadings[middle - 1]) / 2.0f;
    } else {
      weight = scaleReadings[middle];
    }
    isReady = 1;

    //Compose sensor name from client_id
    char tempName[6] = {0};
    memcpy_P(tempName, client_id, sizeof(tempName) - 1);
    snprintf(loadCell->typeName, strlen("scale-") + sizeof(tempName), "%s%s", "scale-", tempName);
    loadCell->value = weight / 1000.0;
  }

  loadCell->canSend = isReady;
}
//-----------------------------------

//------------DS18x20----------------
void readDS18x20() {
  ds.registerDsSensors(dsSensors);
  ds.measureMultipleDsSensors(dsSensors);
}
//-----------------------------------

//------------ADC voltage------------------
void readVoltage(struct SAMSSensor* bat) {
  analogRead(A0);
  int16_t sum = 0;
  int8_t times = 10;
  for (int8_t i = 0; i < times; i++) {
    sum += analogRead(A0);
  }
  float adcReading = ((float)sum / times);
  float volts = adcReading * voltageFactor;
  bat->canSend = 1;
  char tempName[6] = {0};
  memcpy_P(tempName, client_id, sizeof(tempName) - 1);
  snprintf(bat->typeName, strlen("bat-") + sizeof(tempName), "%s%s", "bat-", tempName);
  bat->value = volts;
}
//-----------------------------------

int8_t testSensorPullup(int8_t pinNum) {
  pinMode(pinNum, OUTPUT);
  digitalWrite(pinNum, 0);
  pinMode(pinNum, INPUT);
  delay(1);
  int8_t pinStatus = digitalRead(pinNum);//Check if pullup is working (input should be 1)
  return pinStatus;
}

void startMeasure() {
  if (DS18_ENABLED) {
    if (testSensorPullup(DSPIN) == 1) {
      readDS18x20();
    }
  }

  if (DHT_ENABLED) {
    if (testSensorPullup(DHTPIN) == 1) {
      readDHT22(&sensors[0], &sensors[1]);
    }
  }

  if (SCALE_ENABLED) {
    readScaleHX711(&sensors[2]);
  }

  if (BAT_ENABLED) {
    readVoltage(&sensors[3]);
  }
}

int8_t prepareAllData() {
  //Add 1-wire sensors to all sensor array
  int8_t j = INDIVIDUALSENSORS;
  int8_t arrLength = sizeof(sensors) / sizeof(SAMSSensor);
  int8_t somethingToSend = 0;

  for (int8_t i = 0; i < DS_SENSOR_COUNT; i++)  {
    if (dsSensors[i].up && j < arrLength)    {
      sensors[j].value = dsSensors[i].value;
      snprintf(sensors[j].typeName, sizeof(dsSensors[i].codeHex) + strlen("ds18-"), "%s%s", "ds18-", dsSensors[i].codeHex);
      if (sensors[j].value != 85)
      {
        sensors[j].canSend = 1;
        somethingToSend = 1;
      }
    }
    j++;
  }

  //If no ds18 sensor has data, check if other sensors have data, otherwise don't send anything
  if (!somethingToSend) {
    for (j = 0; j < arrLength; j++) {
      if (sensors[j].canSend) {
        return 1;
      }
    }
  }

  return somethingToSend;
}

//------------REQUESTS----------------
void sendRequestPart(WiFiClientSecure & secureClient, char tokenRTC[], size_t jsonLength, bool bearer) {
  secureClient.print(FPSTR(host));
  secureClient.print(F(":"));
  secureClient.print(httpsPort);
  secureClient.print(FPSTR(acceptPart));
  secureClient.print(jsonLength);
  secureClient.print(FPSTR(contentTypePart));
  if (bearer) {
    secureClient.print(FPSTR(bearerPart));
    secureClient.print(tokenRTC);
  }
  secureClient.print(FPSTR(connClosePart));
}

int8_t requestToken(char* tokenRTC) {
  StaticJsonDocument<256> doc;
  doc["grant_type"] = FPSTR(grantType);
  doc["client_id"] = FPSTR(client_id);
  doc["client_secret"] = FPSTR(secret);
  doc["audience"] = FPSTR(audience);

  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  if (!secureClient.connect(FPSTR(host), httpsPort)) {
    return 0;
  }

  secureClient.print(FPSTR(apiToken));
  sendRequestPart(secureClient, "", measureJson(doc), false);
  char payload[measureJson(doc) + 1];
  serializeJson(doc, payload, sizeof(payload));
  secureClient.print(payload);

  char serverResp[16] = {0};
  secureClient.readBytesUntil('\r', serverResp, sizeof(serverResp));
  if (strstr(serverResp, "HTTP/1.1 200") == NULL) {
    return 0;
  }

  char fullHeader[] = "\r\n\r\n";
  if (!secureClient.find(fullHeader)) {
    return 0;
  }

  //Server sends some weird bytes before json data
  while (secureClient.read() != '\n');

  //Size calculated using https://arduinojson.org/v6/assistant/
  DynamicJsonDocument jsonDoc(512);
  DeserializationError err = deserializeJson(jsonDoc, secureClient.readStringUntil('\r'));
  secureClient.stop();

  if (err) {
    return 0;
  }

  snprintf(tokenRTC, TOKENLENGTH, "%s", jsonDoc["access_token"].as<char *>());

  return 1;
}

int sendToDWH(char tokenRTC[]) {
  //Size can be calculated also here: https://arduinojson.org/v6/assistant/
  DynamicJsonDocument jsonDoc(1024);
  uint8_t arrLength = sizeof(sensors) / sizeof(SAMSSensor);
  for (uint8_t i = 0; i < arrLength; i++) {
    if (sensors[i].canSend) {
      JsonObject obj = jsonDoc.createNestedObject();
      obj["sourceId"] = sensors[i].typeName;
      obj["tint"] = SLEEPTIME;
      JsonArray values = obj.createNestedArray("values");
      JsonObject value = values.createNestedObject();
      value["value"] = sensors[i].value;
    }
  }

  int16_t statusCode = -1;
  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  if (secureClient.connect(FPSTR(host), httpsPort)) {
    secureClient.print(FPSTR(apiHost));
    sendRequestPart(secureClient, tokenRTC, measureJson(jsonDoc), true);
    char payload[measureJson(jsonDoc) + 1];
    serializeJson(jsonDoc, payload, sizeof(payload));
    secureClient.print(payload);

    char serverResp[16] = {0};
    secureClient.readBytesUntil('\r', serverResp, sizeof(serverResp));
    secureClient.stop();

    //Check server response code
    if (strstr(serverResp, "HTTP/1.1 200") != NULL) {
      statusCode = 200;
    } else if (strstr(serverResp, "HTTP/1.1 401") != NULL) {
      statusCode = 401;
    }
  }

  return statusCode;
}

void getRSSI(char signalStr[], uint8_t arrSize) {
  int32_t rssi = WiFi.RSSI();
  if (rssi >= 0) {
    snprintf(signalStr, arrSize, "%s", "N/A");
  } else  if (rssi > -60 && rssi < 0)  {
    snprintf(signalStr, arrSize, "%s", "Excellent");
  }  else if (rssi > -70 && rssi <= -60)  {
    snprintf(signalStr, arrSize, "%s", "Very good");
  }  else if (rssi > -80 && rssi <= -70)  {
    snprintf(signalStr, arrSize, "%s", "OK");
  }  else {
    snprintf(signalStr, arrSize, "%s", "Not good");
  }
}

uint8_t sendLog(char tokenRTC[]) {
  char signalStr[11];
  getRSSI(signalStr, sizeof(signalStr));
  char logMessage[32] = {0};
  snprintf(logMessage, sizeof(VERSION_NUM) + strlen("; Signal: ") + strlen(signalStr), "%s%s%s", VERSION_NUM, "; Signal: ", signalStr);

  StaticJsonDocument<64> doc;
  doc["message"] = logMessage;
  doc["level"] = "info";
  uint8_t statusCode = 0;

  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  if (secureClient.connect(FPSTR(host), httpsPort)) {
    secureClient.print(FPSTR(apiLogs));
    sendRequestPart(secureClient, tokenRTC, measureJson(doc), true);

    char payload[measureJson(doc) + 1];
    serializeJson(doc, payload, sizeof(payload));
    secureClient.print(payload);

    char serverResp[16] = {0};
    secureClient.readBytesUntil('\r', serverResp, sizeof(serverResp));
    secureClient.stop();

    if (strstr(serverResp, "HTTP/1.1 204") != NULL) {
      statusCode = 1;
    }
  }

  return statusCode;
}
//-----------------------------------

void shutdownWiFi() {
  //Power consumption reduction suggestions: https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-1/
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay(1);
}

void wakeWiFi() {
  WiFi.forceSleepWake();
  delay(10);
  //Don't save any WiFi info in the flash memory
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(FPSTR(ssid), FPSTR(pwd));
}

void connectWiFi() {
  int8_t wifiCounter = 0;
  //If can't connect to router, force sleep
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (wifiCounter++ > 20) {
      gotoSleep();
    }
  }
}

void disconnectWiFi()
{
  WiFi.disconnect(true);
  delay(1);
}

void registerRTCVariables(char tokenRTC[], int arrSize, uint8_t &sendFirstLog) {
  //Register variables for RTC memory
  for (int i = 0; i < arrSize; i++) {
    rtcState.registerVar(&(tokenRTC[i]));
  }

  rtcState.registerVar(&sendFirstLog);
}

void gotoSleep() {
  ESP.deepSleep(SLEEPTIME * 1000000, WAKE_RF_DISABLED);
}

//For debug purposes
void printData() {
  uint8_t arrLength = sizeof(sensors) / sizeof(SAMSSensor);
  for (uint8_t i = 0; i < arrLength; i++) {
    if (sensors[i].canSend) {
      Serial.print(sensors[i].typeName);
      Serial.print(F(" | "));
      Serial.println(sensors[i].value);
    }
  }
}

void setup() {
  shutdownWiFi();
  //Uncomment for debug purposes
  //Serial.begin(115200);
  //Serial.println();

  char tokenRTC[TOKENLENGTH];
  uint8_t sendFirstLog;
  uint8_t rtcHasData = 0;

  //Init sensors
  if (DHT_ENABLED) {
    dht.begin();
  }

  if (SCALE_ENABLED) {
    hxScale.begin(DOUT, SCK);
  }

  registerRTCVariables(tokenRTC, sizeof(tokenRTC), sendFirstLog);
  rtcHasData = rtcState.loadFromRTC();

  //Check, why reset happened
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();

  //0 - Power reboot; 6 - Hardware reset
  if (resetInfo->reason == 0 || resetInfo->reason == 6) {
    //First run, some delay for sensor warm-up, send log
    sendFirstLog = 1;
    //Read 1-wire sensors, to perform temp. conversion after power-up
    if (DS18_ENABLED) {
      ds.registerDsSensors(dsSensors);
      ds.measureMultipleDsSensors(dsSensors);
      //Temp. conversion could take up to ~800ms
      delay(1000);
    }
  }

  startMeasure();

  if (prepareAllData()) {
    //Uncomment for debug purposes
    //printData();
    wakeWiFi();
    connectWiFi();

    if (!rtcHasData) {
      if (!requestToken(tokenRTC))
      {
        disconnectWiFi();
        gotoSleep();
      }
      sendFirstLog = 1;
    }

    //Send data to DW and check response
    if (sendToDWH(tokenRTC) == 401) {
      //Probably token expired, request new one
      if (requestToken(tokenRTC)) {
        //Try sending again
        sendToDWH(tokenRTC);
      }
    }

    //Send log if device had power reboot
    if (sendFirstLog) {
      sendFirstLog = !sendLog(tokenRTC);
    }
    disconnectWiFi();
  }
  rtcState.saveToRTC();
  gotoSleep();
}

void loop() {}
