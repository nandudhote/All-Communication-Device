#include "RtosManager.h"  // Header for RtosManager class (task & queue handling)
#include "Config.h"
#include "myWIFI.h"
// #include "w5500.h"
// #include "loraEsp.h"
#include "i2c.h"
#include "internalDrivers.h"
#include "accessPoint.h"
#include "SerialPort.h"  // Custom serial port abstraction (UART with LTE module)
#include "a7670cLTE.h"   // LTE modem driver class
#include <Preferences.h>
#include <ArduinoJson.h>

a7670cLTE lte;
ACCESSPOINT AP;
myWIFI wiFI;
internalDrivers iDrivers;
subPubTopics Topic;
// subPubTopicsW TopicE;
// W5500 w5500;
i2c i2cMaster;
// loraEsp loraMaster;

Preferences prefs;
int entryCount = 0;  // tracks how many I2C readings stored this cycle

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

  while (true) {

    if (SerialPort.available()) {
      String data = SerialPort.readStringUntil('\n');
      data.trim();
      processLine(data);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void RtosManager::txTaskLoop() {
  String receivedData;

  // ── Recover entryCount from flash on boot ──
  prefs.begin("plcdata", true);
  entryCount = prefs.getInt("count", 0);
  prefs.end();
  Serial.printf("[BOOT] Recovered %d stored entries from flash\n", entryCount);

  // ── On boot: publish any leftover data from previous cycle ──
  if (hasPendingData()) {
    Serial.println("[BOOT] Found unpublished data — will publish once LTE is up");
  }

  while (true) {

    if (xQueueReceive(rxQueue, &receivedData, 10 / portTICK_PERIOD_MS) == pdPASS) {
      Serial.println("TX Task got: " + receivedData);

      if (receivedData == "4") {
        lte.lteBeginWithTCP(ServerMQTT, MqttPort, mqttUserName, mqttUserPassword, deviceID, mqttSubscribeTopic);
        lte.publishMsgOverLTENetwork(mqttPublishTopic, deviceID, "online");
        lteNetworkStable = true;


        // ── On boot: publish leftover data ──
        if (hasPendingData()) {
          Serial.println("[BOOT] LTE up — publishing stored entries...");
          String batch = buildJsonFromPrefs();

          lte.publishMsgOverLTENetwork(mqttPublishTopic, deviceID, batch);
          // Since we can't check return value, clear after attempt
          clearPrefs();
          Serial.println("[BOOT] Old data published and cleared");
        }
      }
    }

    // i2cMaster.requestDataFromSlave();  // Request sensor/data from slave MCU (if connected via UART/I2C/SPI)

    // ── Request I2C data — only append when fresh data actually arrives ──
    bool newData = i2cMaster.requestDataFromSlave();
    if (newData && PublishingData.length() > 0) {
      appendToPrefs(PublishingData);  // ← only called when i2cRequestInterval fires
    }

    if (lteNetworkStable) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 10000) {
        lte.checkMQTTStatus();
        previousMillis = currentMillis;
      }
    }

    // ── 60-second publish cycle ──
    unsigned long currentTime = millis();
    if (currentTime - previousTime >= 60000 || responseOn200Request) {

      if (currentTime - previousTime >= 60000) {
        previousTime = currentTime;
      }

      if (hasPendingData()) {
        String batch = buildJsonFromPrefs();
        Serial.printf("[PUB] Publishing batch of %d entries (%d bytes)\n",
                      entryCount, batch.length());

        if (responseOn200Request) {
          lte.publishMsgOverLTENetwork(mqttPublishTopic, "200", batch);
          responseOn200Request = false;
        } else {
          lte.publishMsgOverLTENetwork(mqttPublishTopic, deviceID, batch);
        }

        clearPrefs();  // clear after publish — new cycle starts fresh
        Serial.println("[PUB] Flash cleared — accumulating new cycle");

      } else {
        Serial.println("[PUB] No data to publish this cycle");
      }
    }

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

// Append one I2C reading into flash as entry[0], entry[1], ...
void RtosManager::appendToPrefs(const String &data) {
  prefs.begin("plcdata", false);
  String key = "e" + String(entryCount);  // e0, e1, e2, ...
  prefs.putString(key.c_str(), data);
  entryCount++;
  prefs.putInt("count", entryCount);  // save count so boot can recover it
  prefs.end();
  Serial.printf("[PREFS] Stored entry %d: %s\n", entryCount - 1, data.c_str());
}

// Read all stored entries and build one JSON array string
String RtosManager::buildJsonFromPrefs() {
  prefs.begin("plcdata", true);
  int count = prefs.getInt("count", 0);

  StaticJsonDocument<8192> doc;
  JsonArray arr = doc.createNestedArray("readings");
  doc["total"] = count;
  doc["deviceID"] = deviceID;
  doc["ts"] = millis();

  for (int i = 0; i < count; i++) {
    String key = "e" + String(i);
    String val = prefs.getString(key.c_str(), "");
    if (val.length() > 0) arr.add(val);
  }

  prefs.end();

  String output;
  serializeJson(doc, output);
  return output;
}

// Wipe all stored entries and reset counter
void RtosManager::clearPrefs() {
  prefs.begin("plcdata", false);
  int count = prefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String key = "e" + String(i);
    prefs.remove(key.c_str());
  }
  prefs.remove("count");
  prefs.putBool("pending", false);
  prefs.end();
  entryCount = 0;
  Serial.println("[PREFS] All entries cleared");
}

bool RtosManager::hasPendingData() {
  prefs.begin("plcdata", true);
  int count = prefs.getInt("count", 0);
  prefs.end();
  return count > 0;
}
