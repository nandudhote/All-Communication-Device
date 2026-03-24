#include "loraEsp.h"
#include "Config.h"
#include "internalDrivers.h"

internalDrivers iilDrivers;
SPIClass spiLoRa(VSPI);

loraEsp::loraEsp() {
}

void loraEsp::loraBegin() {
  spiLoRa.begin(_SCKPin, _MISOPin, _MOSIpin, _SSLoraPin);
  LoRa.setTxPower(TX_P);
  LoRa.setSyncWord(ENCRYPT);
  LoRa.setSPI(spiLoRa);
  LoRa.setPins(_SSLoraPin, _RSTLoraPin, _DIO0LoraPin);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
  }
}

void loraEsp::sendOveLora(String data) {
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();
  Serial.print("Sending data : ");
  Serial.println(data);
}

// Function to check LoRa packets (non-blocking)
void loraEsp::checkLoRaPackets() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleReceivedPacket();  // Handle the received data
  }
}


// Function to handle the received packet
void loraEsp::handleReceivedPacket() {
  // recData = "";
  // while (LoRa.available()) {
  //   recData += (char)LoRa.read();
  // }
  // // Serial.println("Received packet: " + recData);
  // SplitData DecodedMsg = splitStringByColon(recData);
  // SplitData DecodedMsg1 = splitStringByColon(TransmitterID);
  // SplitData DecodedMsg2 = splitStringByColon(globalId);

  // if (DecodedMsg.indexOneData == DecodedMsg1.indexOneData && (DecodedMsg.indexTwoData == DecodedMsg1.indexTwoData || DecodedMsg.indexTwoData == DecodedMsg2.indexTwoData)) {
  //   if (DecodedMsg.indexFourData == "S") {
  //     voltage = iDrivers.averageVol(voltage, sampleCount);                                                                              // AVERAGE OUT THE VOLTAGE WHICH ADDED OVER 1 MINTS DURATION
  //     Current = iDrivers.averageCur(Current, sampleCount);                                                                              // AVERAGE OUT THE CURRENT WHICH ADDED OVER 1 MINTS DURATION
  //     Power = iDrivers.calculatePower(voltage.Phase1, voltage.Phase2, voltage.Phase3, Current.Phase1, Current.Phase2, Current.Phase3);  // CALCULATE THE POWER FROM AVERGE VOLTAGE AND CURRENT
  //     KwhPower = iDrivers.calculatePowerInKWH(KwhPower, Power.Phase1, Power.Phase2, Power.Phase3);
  //     TransmitterID.setCharAt(0, 'R');
  //     // globalId.setCharAt(0, 'R');
  //     String sendingData = TransmitterID + ":" + String(voltage.Phase1) + ":" + String(voltage.Phase2) + ":" + String(voltage.Phase3) + ":" + String(Current.Phase1) + ":" + String(Current.Phase2) + ":" + String(Current.Phase3) + ":" + String(KwhPower.Phase1) + ":" + String(KwhPower.Phase2) + ":" + String(KwhPower.Phase3) + ":" + String(loadState);
  //     sendOveLora(sendingData);
  //     sampleCount = 0;                                                                                            // SET AVERAGE COUNT TO ZERO
  //     voltage.Phase1 = voltage.Phase2 = voltage.Phase3 = Current.Phase1 = Current.Phase2 = Current.Phase3 = 0.0;  // RESET THE INSTANT VOLTAGE AND CURRENT
  //   }
  //   if (DecodedMsg.indexFourData == "L") {
  //     if (DecodedMsg.indexFiveData == "100") {
  //       digitalWrite(_contactorPin, HIGH);  // TURN ON CONTACTOR RELAY
  //       loadState = 1;                      // SET LOADSTATE FLAG TO 1 // THIS FLAG DATA SEND OVER MQTT
  //       gotNewLoadStatus = true;            // SET THIS FLAG TO TRUE FOR STORE DATA IN EEPROM // CALLED THIN IN .INO FILE // NOT CALLING EEPROM WRITE FUNCTION HERE TO AVOID INTERRUPT LATTENCY
  //     } else {
  //       digitalWrite(_contactorPin, LOW);  // TURN OFF CONTACTOR RELAY
  //       loadState = 0;                     // SET LOADSTATE FLAG TO 0 // THIS FLAG DATA SEND OVER MQTT
  //       gotNewLoadStatus = true;           // SET THIS FLAG TO TRUE FOR STORE DATA IN EEPROM // CALLED THIN IN .INO FILE // NOT CALLING EEPROM WRITE FUNCTION HERE TO AVOID INTERRUPT LATTENCY
  //     }
  //     recData.setCharAt(0, 'R');
  //     sendOveLora(recData);
  //   }
  // }


  // ledState = (ledState == 0) ? 1 : 0;
  // digitalWrite(_statusLEDPin, ledState);  // TRIGGER STATUS LED ACCORDINGLY STATUS
}

// LoRa reset function
void loraEsp::resetAndResetLoRa() {
  LoRa_reset();
  // Serial.println("LoRa Receiver");
  loraBegin();
}

void loraEsp::LoRa_reset() {
  digitalWrite(_RSTLoraPin, LOW);
  delay(100);
  digitalWrite(_RSTLoraPin, HIGH);
  delay(100);
}