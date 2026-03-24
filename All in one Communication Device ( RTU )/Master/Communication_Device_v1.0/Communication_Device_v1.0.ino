#include "RTOSManager.h"
#include "SerialPort.h"
#include "myWIFI.h"

RtosManager rtos;  // Create an object of RtosManager class to access all rtos-related operations
myWIFI wiFI;

void setup() {
  Serial.begin(9600);
  SerialPort.begin(115200, SERIAL_8N1, RXD2, TXD2);
  rtos.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}
