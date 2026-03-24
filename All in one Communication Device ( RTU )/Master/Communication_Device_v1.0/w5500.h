#ifndef W5500_H_
#define W5500_H_

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

struct subPubTopics {
  String Subscribe;
  String Publish;
};

class W5500 {

public:
  W5500();
  //    static String MqttRecdMsg;
  void Begin(byte EthMAC[], const char* MqttSever, int MqttPort, const char _SSPin);
  void MqttBegin(const char* MqttSever, int mqttPort);
  bool ethernetLinkCheck();
  void reconnectToMqttFromEthernet(String pubTopic, String subTopic, String globalTopic);
  static void MQTT_Pull(char* topic, byte* payload, unsigned int length);
  static void decodeAndUpdateThresholdFromMqtt(String mqttMsg);
  bool CheckMQTTConnection();
  void clientLoop();
  void publishMqttMsg(String pubTopic, String devID, String status);
  void publishMqttMsg_Alert(String pubTopic, String devID, String Alert);
  subPubTopics createSubPubTopics(String devID, String SubTopic, String PubTopic);
  void subscribeTopics(String subsTopic);
};

#endif