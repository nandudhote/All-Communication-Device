#ifndef W5500_H_
#define W5500_H_

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

struct subPubTopicsW {
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
  static void decodeAndUpdateThresholdFromMqttThroughEthernet(String mqttMsg);
  bool CheckMQTTConnectionThroughEthernet();
  void clientLoopThroughEthernet();
  void publishMqttMsgThroughEthernet(String pubTopic, String devID, String status);
  void publishMqttMsg_AlertThroughEthernet(String pubTopic, String devID, String Alert);
  subPubTopicsW createSubPubTopicsThroughEthernet(String devID, String SubTopic, String PubTopic);
  void subscribeTopicsThroughEthernet(String subsTopic);
};

#endif