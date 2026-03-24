#ifndef LORAESP_H_
#define LORAESP_H_

#include <SPI.h>
#include <LoRa.h>

const char _SCKPin = 18;
const char _MISOPin = 19;
const char _MOSIpin = 23;
const char _SSLoraPin = 5;
const char _RSTLoraPin = 14;
const char _DIO0LoraPin = 26;

#define TX_P 17
#define ENCRYPT 0x78

class loraEsp {

public:
  loraEsp();
  void loraBegin();
  void sendOveLora(String data);
  void checkLoRaPackets();
  void handleReceivedPacket();
  void resetAndResetLoRa();
  void LoRa_reset();
};

#endif