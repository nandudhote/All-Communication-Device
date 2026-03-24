#ifndef MY_I2C_MASTER_H
#define MY_I2C_MASTER_H

class MyI2CMaster {

public:
  MyI2CMaster();
  void i2cBegin();
  void i2cScanner();
  void requestDataFromSlave();
  void sendDataToSlave(byte address, String data);
};

#endif