/*
   Library for DS18B20 and DS18S20 sensors

   MIT License
   (c) 2020 Arm-s

*/
#include "Arduino.h"
#include "ds18_20.h"

int8_t DS18OneWire::_divider[] = { 1, 0, 0, 1, 1, 0, 0, 0, 1 }; //polynomial x^8+x^5+x^4+1

DS18OneWire::DS18OneWire(int8_t pin, int8_t sensorCount) {
  _pin = pin;
  //pinMode(_pin, OUTPUT);
  _globalI = 0;
  _sensorCount = sensorCount;
}


int8_t DS18OneWire:: DS_Init(void) {
  cli();
  //Tx reset pulse (0V)
  pinMode(_pin, OUTPUT);
  delayMicroseconds(500);

  //Rx mode (input)
  pinMode(_pin, INPUT);
  //Get signal between (60-240us)
  delayMicroseconds(70);
  int8_t signalVal = digitalRead(_pin);
  delayMicroseconds(410);
  sei();
  return !signalVal;
}

void DS18OneWire::DS_Match_ROM(uint8_t code[]) {
  cli();
  DS_SendByte(0x55);
  sei();

  for (uint8_t i = 0; i < 8; i++) {
    DS_SendByte(code[i]);
  }
}

int8_t DS18OneWire::DS_Search_ROM(uint8_t* done, uint8_t* lastConflict, uint8_t allCode[]) {
  cli();
  uint8_t conflictMarker;
  uint8_t romBitIndx;
  int romBits[64];
  int8_t retVal = 0;

  if (DS_Init()) {

    romBitIndx = 0;
    conflictMarker = 0;
    DS_SendByte(0xf0);

    while (romBitIndx < 64) {
      uint8_t readData = DS_Read();
      uint8_t readNOT = DS_Read();
      if (readData == readNOT && readData == 1) {
        //sei();
        return 0;
      } else if (readData == readNOT && readData == 0) {
        if (*lastConflict == romBitIndx) {
          romBits[romBitIndx] = 1;
        } else if (romBitIndx > *lastConflict) {
          romBits[romBitIndx] = 0;
          conflictMarker = romBitIndx;
        } else if (romBits[romBitIndx] == 0) {
          conflictMarker = romBitIndx;
        }
      } else {
        romBits[romBitIndx] = readData;
      }

      if (romBits[romBitIndx] == 1) {
        DS_WriteHigh();
      } else {
        DS_WriteLow();
      }
      romBitIndx++;
    }

    *lastConflict = conflictMarker;
    if (*lastConflict == 0) {
      *done = 1;
    }
    int romWithoutCRC[64];
    int res = calculateCRC(romWithoutCRC, romBits, _divider, 64);
    if (res) {
      uint8_t j = 0;
      uint8_t k = 0;
      allCode[0] &= ~(allCode[0]);
      for (uint8_t i = 0; i < 64; ++i) {
        //lsb as first bit
        allCode[k] |= (romBits[i] << j);
        ++j;
        if (j == 8) {
          j = 0;
          ++k;
          allCode[k] &= ~(allCode[k]);
        }
      }
      retVal = 1;
    } else {
      retVal = 0;
    }
  }
  sei();
  return retVal;
}

int8_t DS18OneWire::calculateCRC(int romWithoutCRC[], int romBits[], int8_t divider[], uint8_t arrLength) {
  int crc[8];
  uint8_t i = 0;
  uint8_t stopCondition = 0;
  uint8_t indx = -1;
  uint8_t retValue = 1;
  int8_t checkCRC = 1;

  constructRomWithoutCRC(romWithoutCRC, romBits, crc, arrLength);

  while (retValue) {
    while (!stopCondition) {
      if ((++indx + 8) >= arrLength) {
        retValue = 0;
        break;
      }
      stopCondition |= romWithoutCRC[indx] == 1;
    }
    if (retValue) {
      stopCondition = 0;
      for (i = 0; i < 9; i++) {
        romWithoutCRC[i + indx] ^= divider[i];
      }
    }
  }
  //Check if CRC matches
  for (i = 0; i < 8; i++) {
    if (romWithoutCRC[arrLength - 8 + i] != crc[i]) {
      checkCRC = 0;
      break;
    }
  }

  return checkCRC;
}

void DS18OneWire::constructRomWithoutCRC(int romWithoutCRC[], int romBits[], int crc[], uint8_t arrLength) {
  for (uint8_t i = 0; i < arrLength; i++) {
    if (i > arrLength - 9) {
      romWithoutCRC[i] = 0;
      crc[i - (arrLength - 8)] = romBits[i];
    } else {
      romWithoutCRC[i] = romBits[i];
    }
  }
}

int8_t DS18OneWire::DS_ConvertT(void) {
  cli();
  DS_SendByte(0x44);
  sei();
  pinMode(_pin, INPUT);
  while (!digitalRead(_pin)) {
  }

  return 1;
}

int8_t DS18OneWire::DS_ReadScratchpad(uint8_t pad[]) {
  for (uint8_t a = 0; a < 72; a++) {
    _dataBits[a] = 0;
  }

  cli();
  DS_SendByte(0xBE);
  sei();
  pinMode(_pin, INPUT);

  _globalI = 0;
  for (uint8_t i = 0; i < 9; i++) {
    pad[i] = ReadByte();
  }
  pinMode(_pin, INPUT);

  //Define inside
  int arrayWithoutCRC[72];

  return calculateCRC(arrayWithoutCRC, _dataBits, _divider, 72);
}

void DS18OneWire::DS_SendByte(char code) {
  unsigned char shift;
  for (uint8_t i = 0; i < 8; i++) {
    shift = code >> i;
    shift &= 0x01;

    if (shift == 1) {
      DS_WriteHigh();
    } else {
      DS_WriteLow();
    }
    //Between slots at least 1us break
    delayMicroseconds(2);
  }
}

uint8_t DS18OneWire::ReadByte(void) {
  uint8_t i, temp;
  temp = 0;

  for (i = 0; i < 8; i++) {
    if (DS_Read()) {
      temp |= (0x01 << i);
      _dataBits[_globalI] = 1;

    } else {
      temp |= (0x00 << i);
      _dataBits[_globalI] = 0;
    }
    _globalI++;
    delayMicroseconds(15);
  }
  return temp;
}

//==========FOR WRITE SLOTS==========
void DS18OneWire::DS_WriteHigh(void) {
  cli();
  pinMode(_pin, OUTPUT);
  //Bus low, release within 15us
  delayMicroseconds(9);
  pinMode(_pin, INPUT);
  //Wait, so the write sequence is at least 60uS
  delayMicroseconds(64);
  sei();
}

void DS18OneWire::DS_WriteLow(void) {
  cli();
  pinMode(_pin, OUTPUT);
  //Bus low at least 60us (60-120)
  delayMicroseconds(100);
  pinMode(_pin, INPUT);
  sei();
}
//===================================

uint8_t DS18OneWire::DS_Read(void) {
  cli();
  uint8_t value;
  pinMode(_pin, OUTPUT);
  delayMicroseconds(2);
  pinMode(_pin, INPUT);
  delayMicroseconds(13);
  value = digitalRead(_pin);
  delayMicroseconds(55);
  sei();
  return value;
}

void DS18OneWire::registerDsSensors(DsSensor* dsSensors) {
  uint8_t done = 0;
  uint8_t lastConflict = 0;
  uint8_t indx = 0;
  int counter = 0;
  int breakInfinity = _sensorCount * 2;
  while (!done && indx != _sensorCount) {
    if (DS_Search_ROM(&done, &lastConflict, dsSensors[indx].code)) {
      convertHexToString(dsSensors[indx].code, dsSensors[indx].codeHex);
      dsSensors[indx].registered = 1;
      ++indx;
    }
    //If sensor is not connected, get out of infinite loop
    if (counter++ > breakInfinity) {
      return;
    }
    delay(1);
  }
}

void DS18OneWire::measureMultipleDsSensors(DsSensor* dsSensors) {
  for (uint8_t i = 0; i < _sensorCount; i++) {
    if (dsSensors[i].registered) {
      int8_t repeat = 5;
      while (--repeat > 0) {
        if (measureOneSensor(&dsSensors[i]) == 0) {
          repeat = 0;
        }
      }
    }
  }
}

int8_t DS18OneWire:: measureOneSensor(DsSensor* sensor) {
  int8_t repeat = 1;
  if (DS_Init()) {
    DS_Match_ROM(sensor->code);
    if (DS_ConvertT()) {
      if (DS_Init()) {
        DS_Match_ROM(sensor->code);
        if (DS_ReadScratchpad(sensor->scratchPad)) {
          int temp_read = sensor->scratchPad[0];
          if (sensor->code[0] == 0x10) {
            sensor->value = getTemperature10(&temp_read, sensor->scratchPad);
          } else {
            sensor->value = getTemperature28(&temp_read, sensor->scratchPad);
          }
          sensor->up = 1;
          repeat = 0;
          if (sensor->value > 80) {
            repeat = 1;
            sensor->up = 0;
          }
        } else {
          sensor->up = 0;
        }
      }
    }
  }
  return repeat;
}

float DS18OneWire::getTemperature10(int* temp_read, uint8_t temp[]) {
  char sign = 0;
  //Check if MSB is negative: all bits are 1
  if (temp[1] > 0) {
    *temp_read -= 1;
    *temp_read ^= 0xff;
    //*temp_read*=-1;
    sign = 1;
  }

  *temp_read >>= 1;
  float rez = (float) * temp_read - 0.25 + ((float) (temp[7]) - (float) (temp[6])) / (float) (temp[7]);
  if (sign) {
    rez *= -1;
  }
  return rez;
}

float DS18OneWire::getTemperature28(int* temp_read, uint8_t temp[]) {
  char sign = 0;
  if ((temp[1] >> 3) > 1) {
    temp[1] = temp[1] ^ 0xff;
    temp[0] = temp[0] ^ 0xff;
    temp[0] += 1;
    sign = 1;
  }

  uint16_t t = (temp[1] << 8) | temp[0];
  uint16_t dec = t >> 4;
  float part = (temp[0] & 0x0f) / 16.0f;
  float rez = (float)dec + part;

  if (sign) {
    rez *= -1;
  }

  return rez;
}

//Converts 1-wire sensor code to hex chars
void DS18OneWire::convertHexToString(uint8_t arr[], char result[])
{
  uint8_t hex = 0;
  int8_t k = 4;
  int8_t indx = 0;
  int8_t asciiOffset = 0;
  for (int8_t i = 0; i < 8; i++) {
    k = 4;
    for (int8_t j = 7; j >= 0; j--) {
      uint8_t mask = 1 << j;
      uint8_t masked_num = arr[i] & mask;
      uint8_t shiftedBit = masked_num >> k;
      hex |= (shiftedBit);
      if (j == 4 || j == 0) {
        if (hex >= 0 && hex <= 9) {
          asciiOffset = 48;
        } else {
          asciiOffset = 87;
        }
        result[indx++] = hex + asciiOffset;
        k = 0;
        hex = 0;
      }
    }
  }
  result[indx] = '\0';
}
