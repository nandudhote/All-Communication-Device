#include "Config.h"
#include "internalDrivers.h"
#include "i2c.h"
#include <Wire.h>

#define I2C_RESPONSE_SIZE 120  // must match slave

internalDrivers i2cDrivers;

i2c::i2c() {
}

void i2c::i2cBegin() {
  Wire.begin();  // Start I2C in master mode (default SDA/SCL pins)
}

// -----------------------------------------------------------------------------
// Scan all I2C addresses and print detected devices
// -----------------------------------------------------------------------------
void i2c::i2cScanner() {
  byte error, address;
  int nDevices = 0;
  String i2cResult = "I2C devices: ";
  // Serial.println("Scanning for I2C devices ...");

  // Loop through all possible I2C addresses
  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      // Found device
      // Serial.printf("I2C device found at address 0x%02X\n", address);
      i2cResult += "0x" + String(address, HEX) + " ";
      nDevices++;
    } else if (error != 2) {
      // Serial.printf("Error %d at address 0x%02X\n", error, address);  // Error at this address
    }
  }
  // No devices found
  if (nDevices == 0) {
    i2cResult = "No I2C devices found";
    // Serial.println(i2cResult);
  }
  alertMsg = i2cResult;  // Store result in global alertMsg
}

void i2c::requestDataFromSlave() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastRequestTime >= (i2cRequestInterval * 1000)) {
    PublishingData = "";
    lastRequestTime = currentMillis;  // update timestamp

    // --- First STM32 ---
    Wire.requestFrom(i2cAddress, I2C_RESPONSE_SIZE);
    Serial.println("Requesting Slave ");
    String received = "";
    while (Wire.available()) {
      char c = Wire.read();
      if (c == '\0') break;  // Stop at first null terminator (dummy padding)
      received += c;
    }

    if (received.length() > 0) {
      received.trim();
      // digitalWrite(_i2cStatusLED1, HIGH);
      Serial.print("Data received from Slave: ");
      Serial.println(received);
    }
    // delay(1000);
     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // digitalWrite(_i2cStatusLED1, LOW);

    PublishingData = received;
  }
}

// -----------------------------------------------------------------------------
// Send string data to ESP32 slave via I2C
// -----------------------------------------------------------------------------
void i2c::sendDataToSlave(byte address, String data) {
  Wire.beginTransmission(address);
  Wire.write((const uint8_t*)data.c_str(), data.length());  // Send as raw bytes
  Wire.endTransmission();
  Serial.println("Data Send to Slave");
}