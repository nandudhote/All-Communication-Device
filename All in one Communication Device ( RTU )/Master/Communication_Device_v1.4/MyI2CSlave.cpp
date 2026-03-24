#include "MyI2CSlave.h"

MyI2CSlave* MyI2CSlave::instance = nullptr;

MyI2CSlave::MyI2CSlave(uint8_t address) {
  _address = address;
  userCallback = nullptr;
}

void MyI2CSlave::begin() {
  instance = this;

  Wire.begin(_address);
  Wire.onRequest(requestEventStatic);
  Wire.onReceive(receiveEventStatic);
}

void MyI2CSlave::updateData(const char* data) {
  strncpy(_buffer, data, I2C_RESPONSE_SIZE);
}

void MyI2CSlave::onReceiveHandler(void (*func)(SplitData1)) {
  userCallback = func;
}

/* ---------- STATIC WRAPPERS ---------- */

void MyI2CSlave::requestEventStatic() {
  if (instance) instance->requestEvent();
}

void MyI2CSlave::receiveEventStatic(int numBytes) {
  if (instance) instance->receiveEvent(numBytes);
}

/* ---------- ACTUAL HANDLERS ---------- */

void MyI2CSlave::requestEvent() {
  uint8_t buffer[I2C_RESPONSE_SIZE];

  size_t len = strlen(_buffer);
  if (len > I2C_RESPONSE_SIZE) len = I2C_RESPONSE_SIZE;

  memcpy(buffer, _buffer, len);

  for (size_t i = len; i < I2C_RESPONSE_SIZE; i++)
    buffer[i] = 0x00;

  Wire.write(buffer, I2C_RESPONSE_SIZE);
}

void MyI2CSlave::receiveEvent(int numBytes) {
  String receivedData = "";

  while (Wire.available()) {
    char c = Wire.read();
    receivedData += c;
  }

  SplitData1 msg = splitStringByColon(receivedData);

  if (userCallback) {
    userCallback(msg);
  }
}

/* ---------- UTIL ---------- */

SplitData1 MyI2CSlave::splitStringByColon(const String& data) {
  SplitData1 msg;

  int first = data.indexOf(':');

  if (first != -1) {
    msg.indexOneData = data.substring(0, first);

    int second = data.indexOf(':', first + 1);

    if (second != -1) {
      msg.indexTwoData = data.substring(first + 1, second);
      msg.indexThreeData = data.substring(second + 1);
    } else {
      msg.indexTwoData = data.substring(first + 1);
    }
  } else {
    msg.indexOneData = data;
  }

  return msg;
}

/* ---------- RESET ---------- */

void MyI2CSlave::resetI2C() {
  Wire.end();
  delay(10);

  Wire.begin(_address);
  Wire.onRequest(requestEventStatic);
  Wire.onReceive(receiveEventStatic);
}

void MyI2CSlave::i2cBusRecovery() {
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
}