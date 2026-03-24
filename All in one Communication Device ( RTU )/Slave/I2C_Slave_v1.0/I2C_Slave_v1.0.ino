
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
#define _analogPin1 34
#define _analogPin2 35

const char _relayPins[] = { 12, 18, 19, 25 };
extern const byte loadStateEEPROMAddress[] = { 0, 1, 2, 3 };
extern bool relayloadsStatus[] = { 0, 0, 0, 0 };

String powerDataMsg = "";
String mergeData = "";

struct SplitData {
  String indexOneData;
  String indexTwoData;
  String indexThreeData;
};

#define I2C_RESPONSE_SIZE 120  // fixed length to match master request

// <<< I2C watchdog variables
unsigned long lastI2CRequest = 0;
const unsigned long I2C_TIMEOUT1 = 15000;  // 5 seconds no request = reset

void setup() {
  Wire.begin(_slaveAddress);  // Join I2C bus as slave with address 0x08
  EEPROM.begin(512);
  Serial.begin(9600);  // for debugging
                       // RS485
  pinMode(_rs485DIR, OUTPUT);
  digitalWrite(_rs485DIR, LOW);
  Serial2.begin(9600, SERIAL_8N1, _rs485RX, _rs485TX);

  // RS232
  Serial1.begin(9600, SERIAL_8N1, _rs232RX, _rs232TX);

  gpioInit();
  readDataFromEEPROM();
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  lastI2CRequest = millis();  // <<< initialize watchdog
}
void loop() {

  int analog1 = analogRead(_analogPin1);
  int analog2 = analogRead(_analogPin2);

  String rs485Data = "";
  String rs232Data = "";

  // Read RS485
  if (Serial2.available()) {
    rs485Data = Serial2.readStringUntil('\n');
    rs485Data.trim();
  }

  // Read RS232
  if (Serial1.available()) {
    rs232Data = Serial1.readStringUntil('\n');
    rs232Data.trim();
  }

  // Create long string
  mergeData = "RS485" + rs485Data + ":RS232" + rs232Data + ":" + String(analog1) + ":" + String(analog2) + ":" + relayloadsStatus[0] + ":" + relayloadsStatus[1] + ":" + relayloadsStatus[2] + ":" + relayloadsStatus[3];

  // < < < I2C timeout check
  if (millis() - lastI2CRequest > I2C_TIMEOUT1) {
    Serial.println("⚠ I2C inactive, resetting...");
    i2cBusRecovery();  // try to free the bus
    resetI2C();        // restart peripheral
    lastI2CRequest = millis();
  }
}

void gpioInit() {
  for (int i = 0; i < 4; i++) {
    pinMode(_relayPins[i], OUTPUT);
    delay(10);
  }
}

void resetI2C() {
  Serial.println("🔄 Resetting I2C...");

  Wire.end();  // Stop I2C
  delay(10);

  // Reinitialize I2C bus
  Wire.begin(_slaveAddress);  // Join I2C bus as slave with address 0x08
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  delay(10);
  Serial.println("✅ I2C reset complete");
}


void i2cBusRecovery() {
  // adjust pins if using different I2C (this is for I2C1: 22=SCL, 21=SDA)
  pinMode(22, OUTPUT);
  pinMode(21, INPUT);

  for (int i = 0; i < 9; i++) {
    digitalWrite(22, LOW);
    delayMicroseconds(5);
    digitalWrite(22, HIGH);
    delayMicroseconds(5);
  }

  // Generate STOP
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  delayMicroseconds(5);
  digitalWrite(21, HIGH);
  delayMicroseconds(5);

  Serial.println("✅ I2C Bus Recovered");
}

void requestEvent() {
  lastI2CRequest = millis();  // refresh watchdog

  // // Read power data
  // String powerDataMsg = "Hello Master";

  // Prepare fixed-size buffer
  uint8_t buffer[I2C_RESPONSE_SIZE];
  // const char* msg = powerDataMsg.c_str();
  const char* msg = mergeData.c_str();
  size_t len = strlen(msg) + 1;  // include '\0'

  if (len > I2C_RESPONSE_SIZE) len = I2C_RESPONSE_SIZE;  // Truncate if message too long
  memcpy(buffer, msg, len);                              // Copy message into buffer

  // Pad remaining bytes with 0x00
  for (size_t i = len; i < I2C_RESPONSE_SIZE; i++) {
    buffer[i] = 0x00;
  }

  Wire.write(buffer, I2C_RESPONSE_SIZE);  // Send exactly I2C_RESPONSE_SIZE bytes
  // Serial.print("Sent "); Serial.print(I2C_RESPONSE_SIZE); Serial.println(" bytes");
}

void receiveEvent(int numBytes) {
  lastI2CRequest = millis();  // <<< refresh watchdog

  String receivedData = "";  // Clear buffer
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
    bool state = i2cDecodedMsg.indexTwoData.toInt();
    for (int i = 0; i < 4; i++) {
      relayControl(_relayPins[i], state);
      delay(1);
    }
  } else {
    delay(1);
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
  EEPROM.commit();  // Save changes
}


// String powerDataToString(const PowerData1& data1, const PowerData2& data2) {
//   String result = "";
//   // result += String(data1.voltage1, 2) + ":";
//   // result += String(data1.current1, 2) + ":";
//   // result += String(data1.power1, 2) + ":";
//   // result += String(data2.voltage2, 2) + ":";
//   // result += String(data2.current2, 2) + ":";
//   // result += String(data2.power2, 2) + ":";
//   // result += String(relayloadsStatus[0]) + ":";
//   // result += String(relayloadsStatus[1]) + ":";
//   // result += String(relayloadsStatus[2]) + ":";
//   // result += String(relayloadsStatus[3]);
//   return result;
// }

SplitData splitStringByColon(const String& data) {
  SplitData mqttMsg;
  int firstIndex = data.indexOf(':');
  if (firstIndex != -1) {
    mqttMsg.indexOneData = data.substring(0, firstIndex);
    int secondIndex = data.indexOf(':', firstIndex + 1);
    if (secondIndex != -1) {
      mqttMsg.indexTwoData = data.substring(firstIndex + 1, secondIndex);
      mqttMsg.indexThreeData = data.substring(secondIndex + 1);
      if (mqttMsg.indexThreeData.length() > 0) {
      }
    } else {
      mqttMsg.indexTwoData = data.substring(firstIndex + 1);
    }
  } else {
    mqttMsg.indexOneData = data.substring(firstIndex + 1);
  }
  return mqttMsg;
}
