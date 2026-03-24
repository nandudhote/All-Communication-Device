#include <Wire.h>
#include <EEPROM.h>
#include "MyI2CSlave.h"

/* I2C */
MyI2CSlave i2cSlave(0x04);
unsigned long lastI2CRequest = 0;
const unsigned long I2C_TIMEOUT1 = 15000;

/* RS485 */
#define _rs485RX 16
#define _rs485TX 17
#define _rs485DIR 4

#define SLAVE_COUNT 6
#define RS485_TIMEOUT 100

String rs485SlaveData[SLAVE_COUNT];

uint8_t currentSlave = 0;
unsigned long lastPollTime = 0;
unsigned long responseStartTime = 0;
bool waitingResponse = false;

char rs485Buffer[50];
int rs485Index = 0;

/* RS232 */
#define _rs232RX 26
#define _rs232TX 25

char rs232Buffer[50];
int rs232Index = 0;
String rs232Data = "";

unsigned long lastRS232ReceiveTime = 0;
const unsigned long RS232_TIMEOUT = 1000;

/* Analog */
#define _analogPin1 32
#define _analogPin2 33

/* Relays */
const byte _relayPins[] = { 12, 18, 19, 23 };
const byte loadStateEEPROMAddress[] = { 0, 1, 2, 3 };
bool relayloadsStatus[] = { 0, 0, 0, 0 };

/* Merge Data */
#define I2C_RESPONSE_SIZE 120
char mergeData[I2C_RESPONSE_SIZE];

/* ---------- FUNCTION DECLARATIONS ---------- */
void pollRS485Slaves();
void readRS485();
void readRS232();
void gpioInit();
void controlRelay(int index, bool state);
void readDataFromEEPROM();

/* ---------- I2C CALLBACK ---------- */
void handleI2CCommand(SplitData msg) {
  lastI2CRequest = millis();

  bool state = msg.indexTwoData.toInt();

  if (msg.indexOneData == "1") controlRelay(0, state);
  else if (msg.indexOneData == "2") controlRelay(1, state);
  else if (msg.indexOneData == "3") controlRelay(2, state);
  else if (msg.indexOneData == "4") controlRelay(3, state);
  else if (msg.indexOneData == "All") {
    for (int i = 0; i < 4; i++)
      controlRelay(i, state);
  }
}

/* ---------- SETUP ---------- */

void setup() {
  Serial.begin(9600);
  Serial.println("ESP32 Boot");

  EEPROM.begin(512);

  /* I2C INIT */
  i2cSlave.begin();
  i2cSlave.onReceiveHandler(handleI2CCommand);

  /* RS485 */
  pinMode(_rs485DIR, OUTPUT);
  digitalWrite(_rs485DIR, LOW);
  Serial2.begin(9600, SERIAL_8N1, _rs485RX, _rs485TX);

  /* RS232 */
  Serial1.begin(9600, SERIAL_8N1, _rs232RX, _rs232TX);

  gpioInit();
  readDataFromEEPROM();

  lastI2CRequest = millis();
}

/* ---------- LOOP ---------- */

void loop() {
  pollRS485Slaves();
  readRS485();
  readRS232();

  int analog1 = analogRead(_analogPin1);
  int analog2 = analogRead(_analogPin2);

  snprintf(mergeData, sizeof(mergeData),
           "%d:%d:%d:%d:%d:%d:%s:%s:%s:%s:%s:%s:%s",
           analog1,
           analog2,
           relayloadsStatus[0],
           relayloadsStatus[1],
           relayloadsStatus[2],
           relayloadsStatus[3],
           rs485SlaveData[0].c_str(),
           rs485SlaveData[1].c_str(),
           rs485SlaveData[2].c_str(),
           rs485SlaveData[3].c_str(),
           rs485SlaveData[4].c_str(),
           rs485SlaveData[5].c_str(),
           rs232Data.c_str());

  /* Update I2C buffer */
  i2cSlave.updateData(mergeData);

  Serial.println("mergeData : " + String(mergeData));

  /* I2C Watchdog */
  if (millis() - lastI2CRequest > I2C_TIMEOUT1) {
    Serial.println("⚠ I2C inactive, resetting...");
    i2cSlave.i2cBusRecovery();
    i2cSlave.resetI2C();
    lastI2CRequest = millis();
  }

  delay(10);
}

/* ---------- RS485 ---------- */

void sendRS485Command(uint8_t slaveID) {
  digitalWrite(_rs485DIR, HIGH);
  delayMicroseconds(200);

  Serial2.print(slaveID);
  Serial2.println("?");

  Serial2.flush();

  delayMicroseconds(200);
  digitalWrite(_rs485DIR, LOW);

  waitingResponse = true;
  responseStartTime = millis();
}

void pollRS485Slaves() {
  if (!waitingResponse) {
    if (millis() - lastPollTime > 200) {
      currentSlave++;

      if (currentSlave > SLAVE_COUNT) currentSlave = 1;

      sendRS485Command(currentSlave);
      lastPollTime = millis();
    }
  } else {
    if (millis() - responseStartTime > RS485_TIMEOUT) {
      rs485SlaveData[currentSlave - 1] = "NONE";
      waitingResponse = false;
    }
  }
}

void readRS485() {
  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n') {
      rs485Buffer[rs485Index] = '\0';

      String data = String(rs485Buffer);
      data.trim();

      if (currentSlave > 0 && currentSlave <= SLAVE_COUNT)
        rs485SlaveData[currentSlave - 1] = data;

      rs485Index = 0;
      waitingResponse = false;
    } else {
      if (rs485Index < sizeof(rs485Buffer) - 1)
        rs485Buffer[rs485Index++] = c;
    }
  }
}

/* ---------- RS232 ---------- */

void readRS232() {
  while (Serial1.available()) {
    char c = Serial1.read();
    lastRS232ReceiveTime = millis();

    if (c == '\n') {
      rs232Buffer[rs232Index] = '\0';
      rs232Data = String(rs232Buffer);
      rs232Data.trim();
      rs232Index = 0;
    } else {
      if (rs232Index < sizeof(rs232Buffer) - 1)
        rs232Buffer[rs232Index++] = c;
    }
  }

  if (millis() - lastRS232ReceiveTime > RS232_TIMEOUT) {
    rs232Data = "NONE";
  }
}

/* ---------- GPIO ---------- */

void gpioInit() {
  pinMode(_analogPin1, INPUT);
  pinMode(_analogPin2, INPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(_relayPins[i], OUTPUT);
    digitalWrite(_relayPins[i], LOW);
  }
}

/* ---------- RELAY ---------- */

void controlRelay(int index, bool state) {
  digitalWrite(_relayPins[index], state);
  relayloadsStatus[index] = state;

  EEPROM.write(loadStateEEPROMAddress[index], state);
  EEPROM.commit();
}

/* ---------- EEPROM ---------- */

void readDataFromEEPROM() {
  for (int i = 0; i < 4; i++) {
    relayloadsStatus[i] = EEPROM.read(loadStateEEPROMAddress[i]);
    digitalWrite(_relayPins[i], relayloadsStatus[i]);
  }
}