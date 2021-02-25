/*
   Library for DS18B20 and DS18S20 sensors

   MIT License
   (c) 2020 Arm-s

*/
#ifndef DS18_20_H_
#define DS18_20_H_

struct DsSensor {
  uint8_t code[8];
  uint8_t scratchPad[9];
  char codeHex[17]; //NULL terminated
  float value;
  uint8_t up;
  uint8_t registered;
};

class DS18OneWire
{
  private:
    int8_t _pin;
    static int8_t _divider[];
    int _dataBits[72];
    int _globalI;
    int8_t _sensorCount;
    int8_t DS_Init(void);
    void DS_Match_ROM(uint8_t code[]);
    int8_t DS_Search_ROM(uint8_t* done, uint8_t* lastConflict, uint8_t allCode[]);
    int8_t DS_ConvertT(void);
    int8_t DS_ReadScratchpad(uint8_t[]);
    void DS_SendByte(char code);
    void DS_WriteHigh(void);
    void DS_WriteLow(void);
    uint8_t DS_Read(void);
    uint8_t ReadByte(void);
    int8_t calculateCRC(int romWithoutCRC[], int romBits[], int8_t divider[], uint8_t);
    void constructRomWithoutCRC(int romWithoutCRC[], int romBits[], int crc[], uint8_t);
    int8_t measureOneSensor(DsSensor* index);
    float getTemperature10(int* temp_read, uint8_t temp[]);
    float getTemperature28(int* temp_read, uint8_t temp[]);
    void convertHexToString(uint8_t arr[], char result[]);
  public:
    DS18OneWire(int8_t pin, int8_t sensorCount);
    void registerDsSensors(DsSensor* dsSensors);
    void measureMultipleDsSensors(DsSensor* dsSensors);
};

#endif
