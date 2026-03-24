#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

struct subPubTopics {
  String Subscribe;
  String Publish;
  String Global;
  String Alert;
};

class myWIFI {
public:
  myWIFI();
  bool wiFiSetup(String ssid, String pass);
  void mqttSetup(const char* MqttSever, int MqttPort);
  bool wiFiLinkCheck();
  void reconnectToMqtt(String pubTopic, String subTopic, String globalTopic, String alertTopic);
  static void MQTT_Pull(char* topic, byte* payload, unsigned int length);
  bool CheckMQTTConnection();
  void clientLoop();
  void publishMqttMsg(String pubTopic, String devID, String Msg);
  void publishMqttMsg_Alert(String pubTopic, String devID, String Alert);
  subPubTopics createSubPubTopics(String devID, String SubTopic, String PubTopic, String globTopic, String alertTopic);
  String prepareDevID(byte mac[], String devPref);
  void subscribeTopics(String subsTopic);
  void unSubscribe(String subPubTopic);
  void earasWiFiCredentialsFromEEPROM();
};

#endif