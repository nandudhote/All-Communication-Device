#include "W5500.h"
#include "Config.h"
#include "String.h"
#include "internalDrivers.h"
#include "i2c.h"

i2c wi2c;
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
  spiEth.begin(_SCKPin, _MISOPin, _MOSIpin, _SSPin);
  Ethernet.begin(EthMAC);
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
      // Serial.println("ethernet is ok");
      return true;
    } else {
      // Serial.println("link failed");
      return false;
    }
  } else {
    // Serial.println("hardware fault");
    return false;
  }
}

void W5500::reconnectToMqttFromEthernet(String pubTopic, String subTopic, String globalTopic) {
  String willMessage = "{" + deviceId + ":Ethernet:offline}";  // Last Will and Testament message
  if (!eclient.connected()) {
    // Serial.print("Attempting MQTT connection...");
    if (eclient.connect(deviceId.c_str(), mqttUserName, mqttUserPassword, pubTopic.c_str(), 1, true, willMessage.c_str())) {
      // if (eclient.connect(deviceId.c_str(), mqttUserName, mqttUserPassword)) {
      // if (eclient.connect(deviceId.c_str(), pubTopic.c_str(), 1, true, willMessage.c_str())) {
      String onlineMessage = "{" + deviceId + ":Ethernet:online}";
      eclient.publish(pubTopic.c_str(), onlineMessage.c_str(), true);
      // Serial.println("connected");
      ethernetconnectionTrackFlag = true;
      subscribeTopicsThroughEthernet(subTopic);
    } else {
      ethernetconnectionTrackFlag = false;
      // delay(100);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

void W5500::MQTT_Pull(char* topic, byte* payload, unsigned int length) {
  MqttRecdMsg = "";
  for (int i = 0; i < length; i++) {
    if (((char)payload[i] != ' ') && ((char)payload[i] != '\n')) {
      MqttRecdMsg += (char)payload[i];
    }
  }
}

void W5500::decodeAndUpdateThresholdFromMqttThroughEthernet(String mqttMsg) {
  // Serial.print("MqttMsg : ");
  // Serial.println(mqttMsg);
  int delimiterIndex = mqttMsg.indexOf(':');  // Find the index of the delimiter

  SplitData mqttDecodedMsg = iiDrivers.splitStringByColon(mqttMsg);
  if (mqttDecodedMsg.indexOneData == "200") {
    responseOn200Request = true;
  } else if (mqttDecodedMsg.indexOneData == "publishInterval") {
    interval = mqttDecodedMsg.indexTwoData.toInt();
    iiDrivers.writeOneByteInEEPROM(publishIntervalEEPROMAdd, interval);
    alertMsg = "publishInterval is updated : " + String(interval);
  } else if (mqttDecodedMsg.indexOneData == "Relay1" || mqttDecodedMsg.indexOneData == "Relay2" || mqttDecodedMsg.indexOneData == "Relay3" || mqttDecodedMsg.indexOneData == "Relay4") {
    if (mqttDecodedMsg.indexOneData == "Relay1") {
      String relay = "1";
      String msg = relay + ":" + mqttDecodedMsg.indexTwoData;
      wi2c.sendDataToSlave(i2cAddress, msg);
    } else if (mqttDecodedMsg.indexOneData == "Relay2") {
      String relay = "2";
      String msg = relay + ":" + mqttDecodedMsg.indexTwoData;
      wi2c.sendDataToSlave(i2cAddress, msg);
    } else if (mqttDecodedMsg.indexOneData == "Relay3") {
      String relay = "3";
      String msg = relay + ":" + mqttDecodedMsg.indexTwoData;
      wi2c.sendDataToSlave(i2cAddress, msg);
    } else if (mqttDecodedMsg.indexOneData == "Relay4") {
      String relay = "4";
      String msg = relay + ":" + mqttDecodedMsg.indexTwoData;
      wi2c.sendDataToSlave(i2cAddress, msg);
    } else {
      // delay(1);
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  } else if (mqttDecodedMsg.indexOneData == "RelayAll") {
    String relay = "All";
    String msg = relay + ":" + mqttDecodedMsg.indexTwoData;
    wi2c.sendDataToSlave(i2cAddress, msg);
  } else {
    // delay(1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
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
  Topics.Global = devID + globalTopic;
  return Topics;
}

void W5500::subscribeTopicsThroughEthernet(String subsTopic) {
  eclient.subscribe(subsTopic.c_str());
}

String W5500::prepareDevIDThroughEthernet(byte mac[], String devPref) {
  char devID[30];
  snprintf(devID, sizeof(devID), "%s%02X%02X%02X", devPref.c_str(), mac[3], mac[4], mac[5]);
  return String(devID);
}
