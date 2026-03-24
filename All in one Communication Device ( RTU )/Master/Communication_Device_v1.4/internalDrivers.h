#ifndef INTERNALDRIVERS_H
#define INTERNALDRIVERS_H

#include "Arduino.h"
#include <EEPROM.h>

struct SplitData {
  String indexOneData;
  String indexTwoData;
  String indexThreeData;
};

class internalDrivers {
public:
  internalDrivers();
  void gpioInit();
  void readDataFromEEPROM();
  void writeOneByteInEEPROM(int Add, byte data);
  void storeStringInEEPROM(String data, byte Addr);
  String loadStringFromEEPROM(byte Addr, byte Length);
  SplitData splitStringByColon(const String& data);
};

#endif
