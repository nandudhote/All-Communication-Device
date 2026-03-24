#ifndef A7670CLTE_H
#define A7670CLTE_H

#include <Arduino.h>

class a7670cLTE {
public:
  a7670cLTE();
  void lteBeginWithTCP(String mqttServer, int mqttPort, String mqttUserName, String mqttPassword, String deviceId, String subTopic);
  // void publishMqttMessage(String topic, String payload);
  void publishMsgOverLTENetwork(String pubTopic, String devID, String Msg);
  void publishAlertMsgOverLTENetwork(String pubTopic, String devID, String Alert);
  void checkMQTTStatus();
  void sendATCommand(String command);
  void resetLTENetwork();
  void decodeAndUpdateMsgFromMqtt(String mqttMsg);
};

#endif