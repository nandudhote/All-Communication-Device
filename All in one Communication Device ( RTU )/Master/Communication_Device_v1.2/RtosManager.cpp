#include "RtosManager.h"  // Header for RtosManager class (task & queue handling)
#include "Config.h"
#include "myWIFI.h"
#include "w5500.h"
#include "loraEsp.h"
#include "i2c.h"
#include "internalDrivers.h"
#include "accessPoint.h"

ACCESSPOINT AP;
myWIFI wiFI;
internalDrivers iDrivers;
subPubTopics Topic;
subPubTopicsW TopicE;
W5500 w5500;
i2c i2cMaster;
loraEsp loraMaster;

// Init : rxTaskHandle → RX task handler, txTaskHandle → TX task handler, rxQueue → FreeRTOS queue pointer
RtosManager::RtosManager()
  : rxTaskHandle(NULL), txTaskHandle(NULL), rxQueue(NULL) {}

void RtosManager::begin() {
  rxQueue = xQueueCreate(10, sizeof(String));  // Creates a FreeRTOS queue, Can store 10 String objects, Used for RX → TX task communication

  xTaskCreatePinnedToCore(
    rxTask,         // Static RX task function
    "RX Task",      // Task name (for debugging)
    4096,           // Stack size in bytes
    this,           // Pass current object as parameter
    1,              // Task priority
    &rxTaskHandle,  // Store task handle
    1);             // Run on core 1


  xTaskCreatePinnedToCore(
    txTask,         // Static TX task function
    "TX Task",      // Task name
    4096,           // Stack size
    this,           // Pass object reference
    1,              // Priority
    &txTaskHandle,  // Store handle
    0);             // Run on core 0
}

// -------- STATIC TASK WRAPPERS --------
void RtosManager::rxTask(void *pvParameters) {
  RtosManager *self = static_cast<RtosManager *>(pvParameters);  // Converts void* back to RtosManager object
  self->rxTaskLoop();                                            // Calls the real RX task loop
}

void RtosManager::txTask(void *pvParameters) {
  RtosManager *self = static_cast<RtosManager *>(pvParameters);  //Restore object pointer
  self->txTaskLoop();                                            // Calls the real TX task loop
}

// -------- ACTUAL TASK CODE --------
void RtosManager::rxTaskLoop() {
  wiFI.wiFiSetup(SSID, PASSWORD);
  apSSID = wiFI.prepareDevID(MAC, apSSID);
  deviceId = wiFI.prepareDevID(MAC, devNamePrefix);
  deviceAlertId = wiFI.prepareDevID(MAC, alertTopic);
  AP.accessPointSetup();
  wiFI.mqttSetup(ServerMQTT, MqttPort);
  Topic = wiFI.createSubPubTopics(deviceId, subTopic, pubTopic, globalTopic, deviceAlertId);  // establish the connections with Mqtt
  wiFI.reconnectToMqtt(Topic.Publish, Topic.Subscribe, Topic.Global, Topic.Alert);
  Serial.println("WIFI Publish Topic : " + String(Topic.Publish));
  Serial.println("WIFI SubScribe Topic : " + String(Topic.Subscribe));


  while (true) {
    wiFI.clientLoop();

    AP.updateTheSSIDAndPASSFromMqttIfAvailable();
    if (enterInAPMode) {
      mqttCounter = 0;
      wiFI.earasWiFiCredentialsFromEEPROM();
      SSID = "";
      PASSWORD = "";
      AP.accessPointSetup();
      enterInAPMode = false;
    }

    if (wificonnectionTrackFlag) {
      i2cMaster.requestDataFromSlave();  // Request sensor/data from slave MCU (if connected via UART/I2C/SPI)
      unsigned long currentT = millis();
      if (currentT - previousT >= 15000) {
        Serial.println("Prepare Data For Lora Send");
        previousT = currentT;
        TransmitterID.setCharAt(0, 'R');
        globalId.setCharAt(0, 'R');
        String sendingData = TransmitterID + ":" + PublishingData;
        loraMaster.sendOveLora(sendingData);
      }

      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        loraMaster.resetAndResetLoRa();
      }
    }

    /* One Mints Millis loop */
    unsigned long currentTime = millis();
    if (currentTime - previousTime >= (interval * 1000) || responseOn200Request || !isWifiConnected || alertFlag || alertMsg != "") {
      if (wiFI.wiFiLinkCheck()) {
        isWifiConnected = true;
        digitalWrite(_wifiStatusLED, HIGH);
        if (isApStarted) {
          wiFiRetryCounter = 0;
          isApStarted = false;
          AP.stopApServer();
          AP.stopApWiFi();
        }
        if (wiFI.CheckMQTTConnection()) {
          if (alertMsg != "") {
            wiFI.publishMqttMsg_Alert(Topic.Publish, deviceId, alertMsg);
            alertMsg = "";
          }
          if (alertFlag) {
            alertFlag = false;
          }
          if ((currentTime - previousTime >= (interval * 1000)) || responseOn200Request) {
            if (responseOn200Request) {
              if (PublishingData.length() > 0) {
                wiFI.publishMqttMsg(Topic.Publish, "200", PublishingData);
                Serial.println("Wifi Data Publish");
              }
              responseOn200Request = false;
            } else {
              if (PublishingData.length() > 0) {
                wiFI.publishMqttMsg(Topic.Publish, deviceId, PublishingData);
                Serial.println("Wifi Data Publish");
              }
              previousTime = currentTime;
            }
          }
        } else {
          if (!wiFI.CheckMQTTConnection()) {
            wiFI.reconnectToMqtt(Topic.Publish, Topic.Subscribe, Topic.Global, Topic.Alert);
          } else {
            vTaskDelay(100 / portTICK_PERIOD_MS);
          }
        }
      } else {
        wiFI.wiFiSetup(SSID, PASSWORD);
        wiFiRetryCounter += 1;
        if (wiFiRetryCounter >= 2 && (!isApStarted)) {
          AP.readSsidAndPasswordFromAP(apSSID.c_str(), apPassword.c_str());
          isApStarted = true;
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void RtosManager::txTaskLoop() {
  w5500.Begin(MAC, ServerMQTT, MqttPort, _SSPin);
  deviceId = w5500.prepareDevIDThroughEthernet(MAC, devNamePrefix);
  TopicE = w5500.createSubPubTopicsThroughEthernet(deviceId, subTopic, pubTopic);
  w5500.reconnectToMqttFromEthernet(TopicE.Publish, TopicE.Subscribe, TopicE.Global);  // establish the connections with Mqtt
  Serial.println("Ethernet Publish Topic : " + String(TopicE.Publish));
  Serial.println("Ethernet SubScribe Topic : " + String(TopicE.Subscribe));

  while (true) {
    if (ethernetconnectionTrackFlag) {
      i2cMaster.requestDataFromSlave();  // Request sensor/data from slave MCU (if connected via UART/I2C/SPI)
      unsigned long currentT = millis();
      if (currentT - previousT >= 15000) {
        Serial.println("Prepare Data For Lora Send");
        previousT = currentT;
        TransmitterID.setCharAt(0, 'R');
        globalId.setCharAt(0, 'R');
        String sendingData = TransmitterID + ":" + PublishingData;
        loraMaster.sendOveLora(sendingData);
      }

      // unsigned long currentMillis = millis();
      // if (currentMillis - previousMillis >= interval) {
      //   previousMillis = currentMillis;
      //   loraMaster.resetAndResetLoRa();
      // }
    }

    /* One Mints Millis loop */
    if (w5500.ethernetLinkCheck()) {
      ethernetReconnectCounterTimeout = 0;
      ethernetLinkCheckFlag = true;
      w5500.clientLoopThroughEthernet();
      if (MqttRecdMsg != "") {
        w5500.decodeAndUpdateThresholdFromMqttThroughEthernet(MqttRecdMsg);
        MqttRecdMsg = "";
      }
      unsigned long currentTime = millis();
      if (currentTime - previousTime >= (interval * 1000) || responseOn200Request || alertMsg != "") {
        if (w5500.CheckMQTTConnectionThroughEthernet()) {
          mqttRestartCounter = 0;
          if (alertMsg != "") {
            wiFI.publishMqttMsg_Alert(Topic.Publish, deviceId, alertMsg);
            alertMsg = "";
          }
          if (alertFlag) {
            alertFlag = false;
          }
          if ((currentTime - previousTime >= (interval * 1000)) || responseOn200Request) {
            if (responseOn200Request) {
              if (PublishingData.length() > 0) {
                w5500.publishMqttMsgThroughEthernet(TopicE.Publish, "200", PublishingData);
                Serial.println("Ethernet Data Publish");
              }
              responseOn200Request = false;
            } else {
              if (PublishingData.length() > 0) {
                w5500.publishMqttMsgThroughEthernet(TopicE.Publish, deviceId, PublishingData);
                Serial.println("Ethernet Data Publish");
              }
              previousTime = currentTime;
            }
          }

        } else {
          w5500.reconnectToMqttFromEthernet(TopicE.Publish, Topic.Subscribe, Topic.Global);
          mqttRestartCounter++;
          if (mqttRestartCounter >= 200) {
            w5500.Begin(MAC, ServerMQTT, MqttPort, _SSPin);
            mqttRestartCounter = 0;
          }
        }
      }
    } else {
      ethernetLinkCheckFlag = false;
      ethernetReconnectCounterTimeout++;
      if (ethernetReconnectCounterTimeout >= 200) {  // Ethernet will reset after 10 Min of mqtt or ethernet failed
        digitalWrite(_ethernetResetPin, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(_ethernetResetPin, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        w5500.Begin(MAC, ServerMQTT, MqttPort, _SSPin);
        ethernetReconnectCounterTimeout = 0;
        ethernetConnectivityFlag = true;
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
