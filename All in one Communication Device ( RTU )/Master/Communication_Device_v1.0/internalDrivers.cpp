#include "internalDrivers.h"  // Include the header file for internalDrivers class (contains function declarations and constants)
#include "Config.h"           // Include configuration definitions like pins, device IDs, thresholds, etc.
#include <Wire.h>             // I2C communication library

internalDrivers iiiiDrivers;

internalDrivers::internalDrivers() {
}


void internalDrivers::gpioInit() {
  pinMode(_wifiStatusLED, OUTPUT);
}

void internalDrivers::readDataFromEEPROM() {
  EEPROM.begin(512);
  if (((SSID != "") && (PASSWORD != "")) || ((ssidLength != 255) || (passwordLength != 255))) {
    ssidLength = EEPROM.read(ssidLenghtEEPROMAdd);
    passwordLength = EEPROM.read(passwordLenghtEEPROMAdd);
    SSID = loadStringFromEEPROM(ssidEEPROMAdd, ssidLength);
    PASSWORD = loadStringFromEEPROM(passEEPROMAdd, passwordLength);
  }
  byte intervalTemp = EEPROM.read(publishIntervalEEPROMAdd);
  if (intervalTemp != 0) {
    interval = intervalTemp;
  }
}

void internalDrivers::writeOneByteInEEPROM(int Add, byte data) {
  EEPROM.write(Add, data);
  EEPROM.commit();
}

void internalDrivers::storeStringInEEPROM(String data, byte Addr) {
  for (int i = 0; i < data.length(); i++) {
    EEPROM.write(Addr + i, data.charAt(i));
  }
  EEPROM.commit();
}

String internalDrivers::loadStringFromEEPROM(byte Addr, byte Length) {
  String readData = "";
  for (int i = Addr; i < (Addr + Length); i++) {
    readData += char(EEPROM.read(i));
  }
  return readData;
}

SplitData internalDrivers::splitStringByColon(const String& data) {
  SplitData mqttMsg;
  int fi_RSTLoraPinIndex = data.indexOf(':');
  if (fi_RSTLoraPinIndex != -1) {
    mqttMsg.indexOneData = data.substring(0, fi_RSTLoraPinIndex);
    int secondIndex = data.indexOf(':', fi_RSTLoraPinIndex + 1);
    if (secondIndex != -1) {
      mqttMsg.indexTwoData = data.substring(fi_RSTLoraPinIndex + 1, secondIndex);
      mqttMsg.indexThreeData = data.substring(secondIndex + 1);
      if (mqttMsg.indexThreeData.length() > 0) {
      }
    } else {
      mqttMsg.indexTwoData = data.substring(fi_RSTLoraPinIndex + 1);
    }
  } else {
    mqttMsg.indexOneData = data.substring(fi_RSTLoraPinIndex + 1);
  }
  return mqttMsg;
}
