#ifndef CONFIG_H
#define CONFIG_H

#include "Arduino.h"

extern String SSID;
extern String PASSWORD;

extern byte MAC[6];
extern const char* ServerMQTT;
extern int MqttPort;
extern const char* mqttUserName;
extern const char* mqttUserPassword;

extern String devNamePrefix;
extern String alertTopic;
extern String subTopic;  // devName + 6 digits of MAC + /Control
extern String pubTopic;
extern String globalTopic;
extern String apSSID;
extern String apPassword;
extern String deviceId;
extern String deviceAlertId;

extern const byte ssidEEPROMAdd;
extern const byte passEEPROMAdd;
extern const byte ssidLenghtEEPROMAdd;
extern const byte passwordLenghtEEPROMAdd;
extern const byte publishIntervalEEPROMAdd;

extern const byte i2cAddress;  // I2C address of external STM32 slave device1

extern const char _wifiStatusLED;
extern const char _ethernetResetPin;
extern const char _SSPin;
extern const char _SCKPin;
extern const char _MISOPin;
extern const char _MOSIpin;
extern const char _SSLoraPin;
extern const char _RSTLoraPin;
extern const char _DIO0LoraPin;

extern bool responseOn200Request;
extern bool ledState;
extern bool alertFlag;
extern bool enterInAPMode;
extern bool globallySSIDAndPasswordChange;
extern bool espRestartFlag;
extern bool isApStarted;
extern bool isWifiConnected;

extern bool ethernetLinkFlag;
extern byte ethernetResetCounter;

extern unsigned int previousTime;

extern byte ssidLength;
extern byte passwordLength;
extern byte wiFiRetryCounter;
extern byte mqttCounter;
extern byte responseLength;
extern byte interval;

extern int mqttRestartCounter;

extern String publishData;
extern String alertMsg;

extern unsigned long lastRequestTime;  // store last request timestamp
extern unsigned long i2cRequestInterval;        // 5 seconds

extern String PublishingData;

#endif