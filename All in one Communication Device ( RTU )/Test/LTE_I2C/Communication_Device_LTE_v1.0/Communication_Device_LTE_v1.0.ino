#include "RTOSManager.h"
#include "SerialPort.h"
#include "myWIFI.h"
#include "internalDrivers.h"
#include "i2c.h"
#include "loraEsp.h"
#include "a7670cLTE.h"

RtosManager rtos;  // Create an object of RtosManager class to access all rtos-related operations
myWIFI wiFIm;
internalDrivers imDrivers;
i2c i2cmaster;
loraEsp loraMmaster;
// a7670cLTE lte;  // Create an object of a7670cLTE class to access all lte-related operations

void setup() {
  Serial.begin(9600);
  SerialPort.begin(115200, SERIAL_8N1, RXD2, TXD2);
  loraMmaster.loraBegin();
  i2cmaster.i2cBegin();
  imDrivers.readDataFromEEPROM();
  imDrivers.gpioInit();
  rtos.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}
