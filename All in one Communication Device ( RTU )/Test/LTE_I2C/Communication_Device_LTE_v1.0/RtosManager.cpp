#include "RtosManager.h"  // Header for RtosManager class (task & queue handling)
#include "Config.h"
#include "myWIFI.h"
#include "w5500.h"
#include "loraEsp.h"
#include "i2c.h"
#include "internalDrivers.h"
#include "accessPoint.h"
#include "SerialPort.h"  // Custom serial port abstraction (UART with LTE module)
#include "a7670cLTE.h"   // LTE modem driver class

a7670cLTE lte;
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
    rxTask,     // Static RX task function
    "RX Task",  // Task name (for debugging)
    // 4096,           // Stack size in bytes
    8192,           // Stack size in bytes
    this,           // Pass current object as parameter
    1,              // Task priority
    &rxTaskHandle,  // Store task handle
    1);             // Run on core 1


  xTaskCreatePinnedToCore(
    txTask,     // Static TX task function
    "TX Task",  // Task name
    // 4096,           // Stack size
    8192,           // Stack size in bytes
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
  // wiFI.wiFiSetup(SSID, PASSWORD);
  // apSSID = wiFI.prepareDevID(MAC, apSSID);
  // deviceId = wiFI.prepareDevID(MAC, devNamePrefix);
  // deviceAlertId = wiFI.prepareDevID(MAC, alertTopic);
  // AP.accessPointSetup();
  // wiFI.mqttSetup(ServerMQTT, MqttPort);
  // Topic = wiFI.createSubPubTopics(deviceId, subTopic, pubTopic, globalTopic, deviceAlertId);  // establish the connections with Mqtt
  // wiFI.reconnectToMqtt(Topic.Publish, Topic.Subscribe, Topic.Global, Topic.Alert);

  while (true) {

    if (SerialPort.available()) {
      String data = SerialPort.readStringUntil('\n');
      data.trim();
      processLine(data);
    }

    // wiFI.clientLoop();

    // AP.updateTheSSIDAndPASSFromMqttIfAvailable();
    // if (enterInAPMode) {
    //   mqttCounter = 0;
    //   wiFI.earasWiFiCredentialsFromEEPROM();
    //   SSID = "";
    //   PASSWORD = "";
    //   AP.accessPointSetup();
    //   enterInAPMode = false;
    // }

    // if (wificonnectionTrackFlag) {
    //   i2cMaster.requestDataFromSlave();  // Request sensor/data from slave MCU (if connected via UART/I2C/SPI)
    //   unsigned long currentT = millis();
    //   if (currentT - previousT >= 15000) {
    //     // Serial.println("Prepare Data For Lora Send");
    //     previousT = currentT;
    //     TransmitterID.setCharAt(0, 'R');
    //     globalId.setCharAt(0, 'R');
    //     String sendingData = TransmitterID + ":" + PublishingData;
    //     loraMaster.sendOveLora(sendingData);
    //   }

    //   unsigned long currentMillis = millis();
    //   if (currentMillis - previousMillis >= interval) {
    //     previousMillis = currentMillis;
    //     loraMaster.resetAndResetLoRa();
    //   }
    // }

    // /* One Mints Millis loop */
    // unsigned long currentTime = millis();
    // if (currentTime - previousTime >= (interval * 1000) || responseOn200Request || !isWifiConnected || alertFlag || alertMsg != "") {
    //   if (wiFI.wiFiLinkCheck()) {
    //     isWifiConnected = true;
    //     digitalWrite(_wifiStatusLED, HIGH);
    //     if (isApStarted) {
    //       wiFiRetryCounter = 0;
    //       isApStarted = false;
    //       AP.stopApServer();
    //       AP.stopApWiFi();
    //     }
    //     if (wiFI.CheckMQTTConnection()) {
    //       if (alertMsg != "") {
    //         wiFI.publishMqttMsg_Alert(Topic.Publish, deviceId, alertMsg);
    //         alertMsg = "";
    //       }
    //       if (alertFlag) {
    //         alertFlag = false;
    //       }
    //       if ((currentTime - previousTime >= (interval * 1000)) || responseOn200Request) {
    //         if (responseOn200Request) {
    //           if (PublishingData.length() > 0) {
    //             wiFI.publishMqttMsg(Topic.Publish, "200", PublishingData);
    //             // Serial.println("Wifi Data Publish");
    //           }
    //           responseOn200Request = false;
    //         } else {
    //           if (PublishingData.length() > 0) {
    //             wiFI.publishMqttMsg(Topic.Publish, deviceId, PublishingData);
    //             // Serial.println("Wifi Data Publish");
    //           }
    //           previousTime = currentTime;
    //         }
    //       }
    //     } else {
    //       if (!wiFI.CheckMQTTConnection()) {
    //         wiFI.reconnectToMqtt(Topic.Publish, Topic.Subscribe, Topic.Global, Topic.Alert);
    //         vTaskDelay(100 / portTICK_PERIOD_MS);
    //       } else {
    //         vTaskDelay(100 / portTICK_PERIOD_MS);
    //       }
    //     }
    //   } else {
    //     wiFI.wiFiSetup(SSID, PASSWORD);
    //     wiFiRetryCounter += 1;
    //     if (wiFiRetryCounter >= 2 && (!isApStarted)) {
    //       AP.readSsidAndPasswordFromAP(apSSID.c_str(), apPassword.c_str());
    //       isApStarted = true;
    //     }
    //   }
    // }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void RtosManager::txTaskLoop() {
  String receivedData;

  // w5500.Begin(MAC, ServerMQTT, MqttPort, _SSPin);
  // deviceId = w5500.prepareDevIDThroughEthernet(MAC, devNamePrefix);
  // TopicE = w5500.createSubPubTopicsThroughEthernet(deviceId, subTopic, pubTopic);
  // w5500.reconnectToMqttFromEthernet(TopicE.Publish, TopicE.Subscribe, TopicE.Global);  // establish the connections with Mqtt

  while (true) {

    if (xQueueReceive(rxQueue, &receivedData, 10 / portTICK_PERIOD_MS) == pdPASS) {
      Serial.println("TX Task got: " + receivedData);

      if (receivedData == "4") {
        lte.lteBeginWithTCP(ServerMQTT, MqttPort, mqttUserName, mqttUserPassword, deviceID, mqttSubscribeTopic);
        lte.publishMsgOverLTENetwork(mqttPublishTopic, deviceID, "online");
        lteNetworkStable = true;
      }
    }

    i2cMaster.requestDataFromSlave();  // Request sensor/data from slave MCU (if connected via UART/I2C/SPI)

    if (lteNetworkStable) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 10000) {
        lte.checkMQTTStatus();
        previousMillis = currentMillis;
      }
    }

    unsigned long currentTime = millis();
    if (currentTime - previousTime >= 60000 || responseOn200Request) {
      if (currentTime - previousTime >= 60000) {
        previousTime = currentTime;  // RESTART TO MEASURE ONE MINUTES INTERVAL IF ONE MINUTES DONE FROM LAST RESET
      }
      if (responseOn200Request) {
        lte.publishMsgOverLTENetwork(mqttPublishTopic, "200", PublishingData);
        responseOn200Request = false;
      } else {
        lte.publishMsgOverLTENetwork(mqttPublishTopic, deviceID, PublishingData);  // Regular publish (every minute)
      }
    }

    // if (ethernetconnectionTrackFlag) {
    //   i2cMaster.requestDataFromSlave();  // Request sensor/data from slave MCU (if connected via UART/I2C/SPI)
    //   unsigned long currentT = millis();
    //   if (currentT - previousT >= 15000) {
    //     // Serial.println("Prepare Data For Lora Send");
    //     previousT = currentT;
    //     TransmitterID.setCharAt(0, 'R');
    //     globalId.setCharAt(0, 'R');
    //     String sendingData = TransmitterID + ":" + PublishingData;
    //     loraMaster.sendOveLora(sendingData);
    //   }
    // }

    // /* One Mints Millis loop */
    // if (w5500.ethernetLinkCheck()) {
    //   ethernetReconnectCounterTimeout = 0;
    //   ethernetLinkCheckFlag = true;
    //   w5500.clientLoopThroughEthernet();
    //   if (MqttRecdMsg != "") {
    //     w5500.decodeAndUpdateThresholdFromMqttThroughEthernet(MqttRecdMsg);
    //     MqttRecdMsg = "";
    //   }
    //   unsigned long currentTime = millis();
    //   if (currentTime - previousTime >= (interval * 1000) || responseOn200Request || alertMsg != "") {
    //     if (w5500.CheckMQTTConnectionThroughEthernet()) {
    //       mqttRestartCounter = 0;
    //       if (alertMsg != "") {
    //         wiFI.publishMqttMsg_Alert(Topic.Publish, deviceId, alertMsg);
    //         alertMsg = "";
    //       }
    //       if (alertFlag) {
    //         alertFlag = false;
    //       }
    //       if ((currentTime - previousTime >= (interval * 1000)) || responseOn200Request) {
    //         if (responseOn200Request) {
    //           if (PublishingData.length() > 0) {
    //             w5500.publishMqttMsgThroughEthernet(TopicE.Publish, "200", PublishingData);
    //             // Serial.println("Ethernet Data Publish");
    //           }
    //           responseOn200Request = false;
    //         } else {
    //           if (PublishingData.length() > 0) {
    //             w5500.publishMqttMsgThroughEthernet(TopicE.Publish, deviceId, PublishingData);
    //             // Serial.println("Ethernet Data Publish");
    //           }
    //           previousTime = currentTime;
    //         }
    //       }

    //     } else {
    //       w5500.reconnectToMqttFromEthernet(TopicE.Publish, Topic.Subscribe, Topic.Global);
    //       mqttRestartCounter++;
    //       if (mqttRestartCounter >= 200) {
    //         w5500.Begin(MAC, ServerMQTT, MqttPort, _SSPin);
    //         mqttRestartCounter = 0;
    //       }
    //     }
    //   }
    // } else {
    //   ethernetLinkCheckFlag = false;
    //   ethernetReconnectCounterTimeout++;
    //   if (ethernetReconnectCounterTimeout >= 200) {  // Ethernet will reset after 10 Min of mqtt or ethernet failed
    //     digitalWrite(_ethernetResetPin, LOW);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     digitalWrite(_ethernetResetPin, HIGH);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     w5500.Begin(MAC, ServerMQTT, MqttPort, _SSPin);
    //     ethernetReconnectCounterTimeout = 0;
    //     ethernetConnectivityFlag = true;
    //   }
    // }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


// Function to process each line of incoming data
void RtosManager::processLine(String line) {
  String mqttRecMsg = "";

  Serial.print("Rec Data: ");
  Serial.println(line);

  if (counter >= 3) {
    lte.resetLTENetwork();
    counter = 0;
  }

  // The response looks like: +CMQTTCONNECT: 0,"tcp://evoluzn.org...
  if (isCheckingStatus && line.indexOf("tcp://") != -1) {
    statusTcpFound = true;
    Serial.println(">> Status Check: TCP URL Found.");
  }

  // 2. When the command finishes with "OK", we check the result
  if (isCheckingStatus && line.indexOf("OK") != -1) {
    // The modem has finished sending the status list.

    if (statusTcpFound) {
      Serial.println(">> Connection is LIVE.");
      counter = 0;
    } else {
      Serial.println(">> Connection is LOST (No TCP URL). Triggering Reconnect...");
      isCheckingStatus = false;  // Reset flag to prevent double triggers
      lteNetworkStable = false;
      counter += 1;
    }

    isCheckingStatus = false;  // Reset the checking flag because the operation is done
  }

  if (line.indexOf("+CMQTTCONNLOST:") != -1) {
    Serial.println("Connection Lost! Reconnecting...");
    lte.resetLTENetwork();
    lteNetworkStable = false;
  }

  if (strstr(line.c_str(), "+CMQTTCONNECT: 0,0")) {
    Serial.println("Connected to Broker Successfully!");
  } else if (strstr(line.c_str(), "+CMQTTSUB: 0,0")) {
    Serial.println("Topic Subscribed Successfully!");
  } else if (strstr(line.c_str(), "+CMQTTPUB: 0,0")) {
    Serial.println("Msg Published Successfully!");
  } else if (strstr(line.c_str(), "PB DONE")) {
    Serial.println("Network Connected!");
    mqttRecMsg = "4";
    if (mqttRecMsg.length() > 0) {
      Serial.println("RX Task received: " + mqttRecMsg);

      // Send to queue for TX task
      if (xQueueSend(rxQueue, &mqttRecMsg, 0) != pdPASS) {
        Serial.println("Queue Full, Dropping message!");
      }
    }
  } else if (strstr(line.c_str(), "lte")) {
    Serial.println("200 Received");
    isCheckingStatus = false;
    lte.decodeAndUpdateMsgFromMqtt(line);
  } else {
    //pass
  }
}
