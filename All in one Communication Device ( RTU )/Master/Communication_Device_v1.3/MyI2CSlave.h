#ifndef MY_I2C_SLAVE_H
#define MY_I2C_SLAVE_H

#include <Arduino.h>
#include <Wire.h>

#define I2C_RESPONSE_SIZE 120

struct SplitData {
  String indexOneData;
  String indexTwoData;
  String indexThreeData;
};

class MyI2CSlave {
public:
  MyI2CSlave(uint8_t address);

  void begin();

  void updateData(const char* data);               // set mergeData
  void onReceiveHandler(void (*func)(SplitData));  // callback

  void resetI2C();
  void i2cBusRecovery();

private:
  static void requestEventStatic();
  static void receiveEventStatic(int numBytes);

  void requestEvent();
  void receiveEvent(int numBytes);

  SplitData splitStringByColon(const String& data);

  static MyI2CSlave* instance;

  uint8_t _address;
  char _buffer[I2C_RESPONSE_SIZE];

  void (*userCallback)(SplitData);
};

#endif