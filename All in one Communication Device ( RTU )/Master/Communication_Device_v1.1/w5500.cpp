#include "W5500.h"
#include "Config.h"
#include "String.h"
#include "internalDrivers.h"

W5500 iw5500;
internalDrivers iiDrivers;
SPIClass spiEth(HSPI);
EthernetServer server(80);
EthernetClient ethClient;
PubSubClient eclient(ethClient);

W5500::W5500() {
}

void W5500::Begin(byte EthMAC[], const char* MqttSever, int mqttPort, const char _ssPin) {
  digitalWrite(_ethernetResetPin, HIGH);  //Enable ethernet module via reset pin
  Ethernet.init(_ssPin);
  /* At initially if ethernet connected and in device on condition if ethernet try to connect for that we need to do ethernet begin for ESP32 else link will not create*/
  Ethernet.begin(EthMAC);
  // spiEth.begin(_SCKPin, _MISOPin, _MOSIpin, _SSPin);
  MqttBegin(MqttSever, mqttPort);
}

void W5500::MqttBegin(const char* MqttSever, int mqttPort) {
  eclient.setServer(MqttSever, mqttPort);
  eclient.setCallback(MQTT_Pull);
}

bool W5500::ethernetLinkCheck() {
  if (Ethernet.hardwareStatus()) {
    if (Ethernet.linkStatus() == LinkON) {
      /* At initially if ethernet not connected and in device on condition if ethernet try to connect for that we need to do ethernet begin for STM32 else link will not create*/
      if (ethernetLinkFlag) {
        Ethernet.begin(MAC);
        ethernetLinkFlag = false;
      }
      Serial.println("ethernet is ok");
      return true;
    } else {
      Serial.println("link failed");
      return false;
    }
  } else {
    Serial.println("hardware fault");
    return false;
  }
}

// void W5500::reconnectToMqtt(String subTopic) {
//   if (!client.connected()) {
//     Serial.print("Attempting MQTT connection...");
//     if (client.connect(iiDrivers.prepareDevID(MAC, devNamePrefix).c_str())) {
//       Serial.println("connected");
//       subscribeTopicsThroughEthernet(subTopic);
//     } else {
//       delay(100);
//       Serial.print("failed, rc=");
//       Serial.print(client.state());
//       Serial.println(" try again in 5 seconds");
//     }
//   }
// }

void W5500::reconnectToMqttFromEthernet(String pubTopic, String subTopic, String globalTopic) {
  String willMessage = "{" + deviceId + ":offline}";  // Last Will and Testament message
  if (!eclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (eclient.connect(deviceId.c_str(), mqttUserName, mqttUserPassword, pubTopic.c_str(), 1, true, willMessage.c_str())) {
      // if (eclient.connect(deviceId.c_str(), pubTopic.c_str(), 1, true, willMessage.c_str())) {
      String onlineMessage = "{" + deviceId + ":online}";
      eclient.publish(pubTopic.c_str(), onlineMessage.c_str(), true);
      Serial.println("connected");
      subscribeTopicsThroughEthernet(subTopic);
    } else {
      delay(100);
    }
  }
}

void W5500::MQTT_Pull(char* topic, byte* payload, unsigned int length) {
  String MqttRecdMsg = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    if (((char)payload[i] != ' ') && ((char)payload[i] != '\n')) {
      MqttRecdMsg += (char)payload[i];
      Serial.print((char)payload[i]);
    }
  }
  Serial.println();
  decodeAndUpdateThresholdFromMqttThroughEthernet(MqttRecdMsg);
}

void W5500::decodeAndUpdateThresholdFromMqttThroughEthernet(String mqttMsg) {
  Serial.print("MqttMsg : ");
  Serial.println(mqttMsg);
  int delimiterIndex = mqttMsg.indexOf(':');  // Find the index of the delimiter

  if (mqttMsg == "200") {
    responseOn200Request = true;
  }
}

bool W5500::CheckMQTTConnectionThroughEthernet() {
  if (eclient.connected()) {
    return true;
  }
  return false;
}

void W5500::clientLoopThroughEthernet() {
  eclient.loop();
}

void W5500::publishMqttMsgThroughEthernet(String pubTopic, String devID, String status) {
  String Final;
  Final.concat("{device_id:" + devID + ":");
  Final.concat(status);
  Final.concat("}");
  eclient.publish(pubTopic.c_str(), Final.c_str());
}

void W5500::publishMqttMsg_AlertThroughEthernet(String pubTopic, String devID, String Alert) {
  char Final[50];
  sprintf(Final, "{%s:%s}", devID.c_str(), Alert.c_str());
  eclient.publish(pubTopic.c_str(), Final);
}

subPubTopicsW W5500::createSubPubTopicsThroughEthernet(String devID, String SubTopic, String PubTopic) {
  subPubTopicsW Topics;
  Topics.Subscribe = devID + SubTopic;
  Topics.Publish = devID + PubTopic;
  return Topics;
}

void W5500::subscribeTopicsThroughEthernet(String subsTopic) {
  eclient.subscribe(subsTopic.c_str());
}
