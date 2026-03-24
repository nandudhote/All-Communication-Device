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

extern String TransmitterID;
extern String globalId;

extern String devNamePrefix;
extern String alertTopic;
extern String subTopic;  // devName + 6 digits of MAC + /Control
extern String pubTopic;
extern String globalTopic;
extern String apSSID;
extern String apPassword;
extern String deviceId;
extern String deviceAlertId;

extern String deviceID;
extern String mqttPublishTopic;
extern String mqttSubscribeTopic;
extern String mqttAlertPublishTopic;

extern const byte ssidEEPROMAdd;
extern const byte passEEPROMAdd;
extern const byte ssidLenghtEEPROMAdd;
extern const byte passwordLenghtEEPROMAdd;
extern const byte publishIntervalEEPROMAdd;

extern const byte i2cAddress;  // I2C address of external STM32 slave device1

extern const char _wifiStatusLED;
extern const char _ethernetResetPin;
extern const char _button;
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
extern bool wificonnectionTrackFlag;
extern bool ethernetconnectionTrackFlag;

extern bool ethernetLinkFlag;
extern bool ethernetConnectivityFlag;
extern bool ethernetLinkCheckFlag;

extern bool isCheckingStatus;  // Are we currently waiting for a status check?
extern bool statusTcpFound;    // Did we find the TCP URL in the response?
extern bool lteBeginFailed;
extern bool lteNetworkStable;

extern unsigned int previousTime;
extern unsigned long previousT;
extern const unsigned long intervalT;
extern unsigned long previousMillis;
extern const unsigned long intervalMillis;
extern unsigned int ethernetReconnectCounterTimeout;
extern unsigned long lastRequestTime;     // store last request timestamp
extern unsigned long i2cRequestInterval;  // 5 seconds

extern byte ssidLength;
extern byte passwordLength;
extern byte wiFiRetryCounter;
extern byte mqttCounter;
extern byte responseLength;
extern byte interval;

extern int mqttRestartCounter;

extern String publishData;
extern String alertMsg;

extern String PublishingData;
extern String MqttRecdMsg;

#endif