#include "HX711.h"

#define DOUT 4
#define SCK 5

HX711 hxScale;

float calWeight = 5000;

String input = "";
int8_t stringComplete = 0;

int8_t startMeasure = 0;
int8_t startCalibrate = 0;
int8_t inputWeight = 0;

float scaleFactor = 1.0f;
long scaleOffset=1;

void resetScale(float value) {
  hxScale.set_scale(value);
  hxScale.tare();
}

void calibrateScale() {
  float sum = 0.0f;
  int8_t times = 10;
  for (int8_t i = 0; i < times; i++) {
    long rawWeight = hxScale.get_value(1);
    sum += (rawWeight / calWeight);
    Serial.print(F("."));
    delay(500);
  }
  scaleFactor = sum / times;
  scaleOffset = hxScale.get_offset();
  Serial.println();
  Serial.print(F("Offset: "));
  Serial.print(scaleOffset);
  Serial.print(F(" ScaleFactor: "));
  Serial.println(scaleFactor);
  Serial.println(F("Calibration finished. Remove weight!"));
  startCalibrate = 0;
}

float measure() {
  return hxScale.get_units(10) / 1000.0;
}

void menu() {
  Serial.println();
  Serial.print(F("1 - set known weight | current: "));
  Serial.print(calWeight);
  Serial.println(F("g;"));
  Serial.println(F("2 - calibrate;"));
  Serial.println(F("3 - measure;"));
  Serial.println(F("q - show menu;"));
  Serial.println(F("Choice (1-3): "));
}

void setup() {
  Serial.begin(115200);
  hxScale.begin(DOUT, SCK);
  input.reserve(200);
  menu();
}

void loop() {
  if (Serial.available()) {
    serialEvent();
  }
  if (stringComplete) {
    stringComplete = 0;
    startMeasure = 0;

    if (inputWeight) {
      calWeight = input.toFloat();
      inputWeight = 0;
      Serial.print(calWeight);
      Serial.println(F("g"));
    }

    if (input == "w" && startCalibrate) {
      calibrateScale();
      menu();
    }
    else if (input == "1" && !inputWeight) {
      inputWeight = 1;
      Serial.print(F("Known weight (g):"));
    }
    else if (input == "2" && !startCalibrate) {
      startCalibrate = 1;
      resetScale(0.0f);
      Serial.println(F("Place weight and enter 'w'!"));
    }
    else if (input == "3") {
      startMeasure = 1;
      resetScale(scaleFactor);
      hxScale.set_offset(scaleOffset);
      Serial.println();
      Serial.print(F("Offset: "));
//      Serial.print(hxScale.get_offset());
      Serial.print(scaleOffset);
      Serial.print(F(" ScaleFactor: "));
      Serial.println(scaleFactor);
    }
    else {
      menu();
      startCalibrate = 0;
    }
    input = "";
  }

  if (startMeasure) {
    Serial.print(measure(), 3);
    Serial.println(F(" kg"));
    delay(1000);
  }
  delay(10);
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = 1;
    }
    else if (inChar != '\r') {
      input += inChar;
    }
  }
}
