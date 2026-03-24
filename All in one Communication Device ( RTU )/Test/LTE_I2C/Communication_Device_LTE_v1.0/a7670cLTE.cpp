#include "a7670cLTE.h"
#include "Config.h"
#include "SerialPort.h"
#include "internalDrivers.h"  // Include the internal drivers header. This is where your core logic and class definitions (like sensor handling, alerts, etc.) are declared.

extern internalDrivers iDrivers;  // Create an object of internalDrivers class to access all device-related operations

a7670cLTE::a7670cLTE() {}

// ---------------- LTE COMMANDS FOR LTE BEGIN ----------------
void a7670cLTE::lteBeginWithTCP(String mqttServer, int mqttPort, String mqttUserName, String mqttPassword, String deviceId, String subTopic) {
  Serial.println("--- Starting LTE Connection Sequence ---");
  sendATCommand("ATE0");
  sendATCommand("AT+CMQTTSTART");
  sendATCommand("AT+CMQTTACCQ=0,\"" + deviceId + "\",0");
  String willMsg = "{device_id:" + deviceId + ":" + "offline}";
  sendATCommand("AT+CMQTTWILLTOPIC=0," + String(mqttPublishTopic.length()));
  sendATCommand(mqttPublishTopic);
  sendATCommand("AT+CMQTTWILLMSG=0," + String(willMsg.length()) + ",1");
  sendATCommand(willMsg);
  sendATCommand("AT+CMQTTCONNECT=0,\"tcp://" + mqttServer + ":" + String(mqttPort) + "\",90,1,\"" + mqttUserName + "\",\"" + mqttPassword + "\"");
  sendATCommand("AT+CMQTTSUBTOPIC=0," + String(subTopic.length()) + ",1");
  sendATCommand(subTopic);
  sendATCommand("AT+CMQTTSUB=0," + String(subTopic.length()) + ",1");
  sendATCommand(subTopic);
  Serial.println("LWT Configured & Subscribe Topic Sent");
}

void a7670cLTE::publishMsgOverLTENetwork(String pubTopic, String devID, String Msg) {
  String transmittingString = "{device_id:" + devID + ":" + Msg + "}";        // MERGE ALL SENDING PARAMETERS IN ONE STRING
  sendATCommand("AT+CMQTTTOPIC=0," + String(pubTopic.length()));              // SEND COMMAND TO LOAD PUBLISH TOPIC
  sendATCommand(pubTopic);                                                    // SEND PUBLISH TOPIC
  sendATCommand("AT+CMQTTPAYLOAD=0," + String(transmittingString.length()));  // SEND COMMAND TO LOAD MESSAGE
  sendATCommand(transmittingString);                                          // PAYLOAD MESSAGE
  sendATCommand("AT+CMQTTPUB=0,1,60");                                        // PUBLISH THE MESSAGE
}

void a7670cLTE::publishAlertMsgOverLTENetwork(String pubTopic, String devID, String Alert) {
  String transmittingString = "{device_id:" + devID + ":" + Alert + "}";      // MERGE ALL SENDING PARAMETERS IN ONE STRING
  sendATCommand("AT+CMQTTTOPIC=0," + String(pubTopic.length()));              // SEND COMMAND TO LOAD PUBLISH TOPIC
  sendATCommand(pubTopic);                                                    // SEND PUBLISH TOPIC
  sendATCommand("AT+CMQTTPAYLOAD=0," + String(transmittingString.length()));  // SEND COMMAND TO LOAD MESSAGE
  sendATCommand(transmittingString);                                          // PAYLOAD MESSAGE
  sendATCommand("AT+CMQTTPUB=0,1,60");                                        // PUBLISH THE MESSAGE
}

void a7670cLTE::checkMQTTStatus() {
  Serial.println("Checking MQTT Status...");
  // Reset flags before sending
  isCheckingStatus = true;
  statusTcpFound = false;
  sendATCommand("AT+CMQTTCONNECT?");  // Send the command
}

void a7670cLTE::sendATCommand(String command) {
  SerialPort.println(command);
  delay(500);
  // delay(1000);
}

void a7670cLTE::resetLTENetwork() {
  sendATCommand("AT+CMQTTDISC=0,60");  // DISCONNECT THE MQTT
  sendATCommand("AT+CMQTTREL=0");      // This releases the client index and clears the buffers/credentials from the module's RAM.
  sendATCommand("AT+CMQTTSTOP");       // Stop MQTT service
  sendATCommand("AT+CRESET");          // DO SOFT RESET
  // delay(2000);
}


void a7670cLTE::decodeAndUpdateMsgFromMqtt(String mqttMsg) {
  String msg = "";
  SplitData mqttDecodedMsg = iDrivers.splitStringByColon(mqttMsg);

  if (mqttDecodedMsg.indexTwoData == "200") {
    responseOn200Request = true;
  } else {
    delay(1);
  }
}
