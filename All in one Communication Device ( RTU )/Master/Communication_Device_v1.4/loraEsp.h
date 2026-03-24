#ifndef LORAESP_H_
#define LORAESP_H_

#include <SPI.h>
#include <LoRa.h>

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