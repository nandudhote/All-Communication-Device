#include <sys/_intsup.h>
#include "Config.h"

String SSID = "";
String PASSWORD = "";

byte MAC[6] = { 0x8C, 0x4C, 0xAD, 0xF0, 0xC0, 0x39 };
// const char* ServerMQTT = "203.109.124.70";
const char* ServerMQTT = "evoluzn.org";
int MqttPort = 18889;
const char* mqttUserName = "evzin_led";
const char* mqttUserPassword = "63I9YhMaXpa49Eb";

String TransmitterID = "T:12:01";
String globalId = "T:12:GG";

String devNamePrefix = "RTU";
String alertTopic = "RTUAlert";
String subTopic = "/control";  // devName + 6 digits of MAC + /Control
String pubTopic = "/status";   // // devName + 6 digits of MAC + /Status
String globalTopic = "RTUGlobal";
String apSSID = "RTUAp";
String apPassword = "123456789";
String deviceId = "";
String deviceAlertId = "";

const byte ssidEEPROMAdd = 00;
const byte passEEPROMAdd = 21;
const byte ssidLenghtEEPROMAdd = 41;
const byte passwordLenghtEEPROMAdd = 42;
const byte publishIntervalEEPROMAdd = 43;

const byte i2cAddress = 0x04;  // I2C address of STM32 slave1

const char _wifiStatusLED = 2;
const char _ethernetResetPin = 2;
const char _button = 4;
const char _SSPin = 33;
const char _SCKPin = 18;
const char _MISOPin = 19;
const char _MOSIpin = 23;
const char _SSLoraPin = 5;
const char _RSTLoraPin = 14;
const char _DIO0LoraPin = 26;

bool responseOn200Request = false;
bool ledState = false;
bool alertFlag = false;
bool enterInAPMode = false;
bool globallySSIDAndPasswordChange = false;
bool espRestartFlag = false;
bool isApStarted = false;
bool isWifiConnected = false;
bool wificonnectionTrackFlag = false;
bool ethernetconnectionTrackFlag = false;

bool ethernetLinkFlag = true;
bool ethernetConnectivityFlag = true;
unsigned int ethernetReconnectCounterTimeout = 0;
bool ethernetLinkCheckFlag = false;


unsigned int previousTime = 0;
unsigned long previousT = 0;
const unsigned long intervalT = 2000;
unsigned long previousMillis = 0;
const unsigned long intervalMillis = 60000;  // 5 seconds

byte ssidLength = 0;
byte passwordLength = 0;
byte wiFiRetryCounter = 0;
byte mqttCounter = 0;
byte responseLength = 2;
byte interval = 60;

int mqttRestartCounter = 0;

String publishData = "";
String alertMsg = "";

unsigned long lastRequestTime = 0;      // store last request timestamp
unsigned long i2cRequestInterval = 10;  // 5 seconds

String PublishingData = "";
String MqttRecdMsg = "";
