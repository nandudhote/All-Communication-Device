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
            // wiFI.publishMqttMsg_Alert(Topic.Publish, deviceId, alertMsg);
            alertMsg = "";
          }
          if (alertFlag) {
            alertFlag = false;
          }
          if ((currentTime - previousTime >= (interval * 1000)) || responseOn200Request) {
            if (responseOn200Request) {
              wiFI.publishMqttMsg(Topic.Publish, "200", "200 Response Publish");
              responseOn200Request = false;
            } else {
              wiFI.publishMqttMsg(Topic.Publish, deviceId, "Device Online");
              previousTime = currentTime;
            }
          }
        } else {
          if (!wiFI.CheckMQTTConnection()) {
            wiFI.reconnectToMqtt(Topic.Publish, Topic.Subscribe, Topic.Global, Topic.Alert);
          } else {
            delay(100);
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

  while (true) {

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
