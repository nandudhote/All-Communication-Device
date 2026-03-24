#ifndef I2C_H_
#define I2C_H_

class i2c {

public:
  i2c();
  void i2cBegin();
  void i2cScanner();
  // void requestDataFromSlave();
  bool requestDataFromSlave();
  void sendDataToSlave(byte address, String data);
};

#endif