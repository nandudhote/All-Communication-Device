#include <Wire.h>
#include <EEPROM.h>

#define _slaveAddress 0x04

// RS485
#define _rs485RX 16
#define _rs485TX 17
#define _rs485DIR 4

// RS232
#define _rs232RX 26
#define _rs232TX 25

// Analog
#define _analogPin1 32
#define _analogPin2 33

// Relay pins (changed last pin to 23 to avoid RS232 conflict)
const byte _relayPins[] = { 12, 18, 19, 23 };

const byte loadStateEEPROMAddress[] = { 0, 1, 2, 3 };
bool relayloadsStatus[] = { 0, 0, 0, 0 };

#define I2C_RESPONSE_SIZE 120

char mergeData[I2C_RESPONSE_SIZE];

struct SplitData {
  String indexOneData;
  String indexTwoData;
  String indexThreeData;
};

// I2C watchdog
unsigned long lastI2CRequest = 0;
const unsigned long I2C_TIMEOUT1 = 15000;  // 15 seconds

void setup() {
  delay(2000);
  Serial.begin(9600);
  Wire.begin(_slaveAddress);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  EEPROM.begin(512);

  // RS485
  pinMode(_rs485DIR, OUTPUT);
  digitalWrite(_rs485DIR, LOW);
  Serial2.begin(9600, SERIAL_8N1, _rs485RX, _rs485TX);

  // RS232
  Serial1.begin(9600, SERIAL_8N1, _rs232RX, _rs232TX);

  gpioInit();
  readDataFromEEPROM();
  Serial.println("Inside Setup");

  lastI2CRequest = millis();
}

void loop() {
  int analog1 = analogRead(_analogPin1);
  Serial.println("analog1 : " + String(analog1));
  int analog2 = analogRead(_analogPin2);
  Serial.println("analog2 : " + String(analog2));

  String rs485Data = "";
  String rs232Data = "";

  if (Serial2.available() > 0) {
    rs485Data = Serial2.readStringUntil('\n');
    rs485Data.trim();
  }

  if (Serial1.available() > 0) {
    rs232Data = Serial1.readStringUntil('\n');
    rs232Data.trim();
  }

  if (rs485Data.length() > 0 || rs232Data.length() > 0) {
    snprintf(mergeData, sizeof(mergeData),
             "%d:%d:%d:%d:%d:%d:%s:%s",
             analog1,
             analog2,
             relayloadsStatus[0],
             relayloadsStatus[1],
             relayloadsStatus[2],
             relayloadsStatus[3],
             rs485Data.c_str(),
             rs232Data.c_str());
  } else {
    snprintf(mergeData, sizeof(mergeData),
             "%d:%d:%d:%d:%d:%d",
             analog1,
             analog2,
             relayloadsStatus[0],
             relayloadsStatus[1],
             relayloadsStatus[2],
             relayloadsStatus[3]);
  }

  // I2C timeout watchdog
  if (millis() - lastI2CRequest > I2C_TIMEOUT1) {

    Serial.println("⚠ I2C inactive, resetting...");

    i2cBusRecovery();
    resetI2C();

    lastI2CRequest = millis();
  }
}

void gpioInit() {
  pinMode(_analogPin1, INPUT);
  pinMode(_analogPin2, INPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(_relayPins[i], OUTPUT);
    digitalWrite(_relayPins[i], LOW);
    delay(10);
  }
}

void resetI2C() {

  Serial.println("🔄 Resetting I2C...");

  Wire.end();
  delay(10);

  Wire.begin(_slaveAddress);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  delay(10);

  Serial.println("✅ I2C reset complete");
}

void i2cBusRecovery() {

  pinMode(22, OUTPUT);
  pinMode(21, INPUT);

  for (int i = 0; i < 9; i++) {

    digitalWrite(22, LOW);
    delayMicroseconds(5);
    digitalWrite(22, HIGH);
    delayMicroseconds(5);
  }

  pinMode(21, OUTPUT);

  digitalWrite(21, LOW);
  delayMicroseconds(5);
  digitalWrite(21, HIGH);
  delayMicroseconds(5);

  Serial.println("✅ I2C Bus Recovered");
}

void requestEvent() {

  lastI2CRequest = millis();

  uint8_t buffer[I2C_RESPONSE_SIZE];

  size_t len = strlen(mergeData);

  if (len > I2C_RESPONSE_SIZE)
    len = I2C_RESPONSE_SIZE;

  memcpy(buffer, mergeData, len);

  for (size_t i = len; i < I2C_RESPONSE_SIZE; i++)
    buffer[i] = 0x00;

  Wire.write(buffer, I2C_RESPONSE_SIZE);
}

void receiveEvent(int numBytes) {

  lastI2CRequest = millis();

  String receivedData = "";

  while (Wire.available()) {
    char c = Wire.read();
    receivedData += c;
  }

  Serial.print("Received from master: ");
  Serial.println(receivedData);

  SplitData i2cDecodedMsg = splitStringByColon(receivedData);

  bool state = i2cDecodedMsg.indexTwoData.toInt();

  if (i2cDecodedMsg.indexOneData == "1") {
    relayControl(_relayPins[0], state);
    relayloadsStatus[0] = state;
    writeOneByteInEEPROM(loadStateEEPROMAddress[0], state);
  } else if (i2cDecodedMsg.indexOneData == "2") {
    relayControl(_relayPins[1], state);
    relayloadsStatus[1] = state;
    writeOneByteInEEPROM(loadStateEEPROMAddress[1], state);
  } else if (i2cDecodedMsg.indexOneData == "3") {
    relayControl(_relayPins[2], state);
    relayloadsStatus[2] = state;
    writeOneByteInEEPROM(loadStateEEPROMAddress[2], state);
  } else if (i2cDecodedMsg.indexOneData == "4") {
    relayControl(_relayPins[3], state);
    relayloadsStatus[3] = state;
    writeOneByteInEEPROM(loadStateEEPROMAddress[3], state);
  } else if (i2cDecodedMsg.indexOneData == "All") {
    for (int i = 0; i < 4; i++) {
      relayControl(_relayPins[i], state);
      relayloadsStatus[i] = state;
      writeOneByteInEEPROM(loadStateEEPROMAddress[i], state);
      delay(1);
    }
  } else if (i2cDecodedMsg.indexOneData == "Motion") {
    for (int i = 0; i < 4; i++) {
      relayControl(_relayPins[i], state);
      delay(1);
    }
  }
}

void relayControl(int relayPin, bool state) {

  digitalWrite(relayPin, state);
}

void readDataFromEEPROM() {

  for (int i = 0; i < 4; i++) {

    relayloadsStatus[i] = EEPROM.read(loadStateEEPROMAddress[i]);
    relayControl(_relayPins[i], relayloadsStatus[i]);
    delay(10);
  }
}

void writeOneByteInEEPROM(int Add, byte data) {

  EEPROM.write(Add, data);
  EEPROM.commit();
}

SplitData splitStringByColon(const String& data) {

  SplitData mqttMsg;

  int firstIndex = data.indexOf(':');

  if (firstIndex != -1) {

    mqttMsg.indexOneData = data.substring(0, firstIndex);

    int secondIndex = data.indexOf(':', firstIndex + 1);

    if (secondIndex != -1) {

      mqttMsg.indexTwoData = data.substring(firstIndex + 1, secondIndex);
      mqttMsg.indexThreeData = data.substring(secondIndex + 1);
    }

    else {

      mqttMsg.indexTwoData = data.substring(firstIndex + 1);
    }
  }

  else {

    mqttMsg.indexOneData = data;
  }

  return mqttMsg;
}