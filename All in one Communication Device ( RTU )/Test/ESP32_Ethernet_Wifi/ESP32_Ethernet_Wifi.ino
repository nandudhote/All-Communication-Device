#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// ──────────────────────────────────────────────────────────────
//  Hardware
// ──────────────────────────────────────────────────────────────
#define BUTTON_PIN 4
#define ETH_CS_PIN 33
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

#define SS 5
#define RST 14
#define DIO0 26

SPIClass spiLoRa(VSPI);
SPIClass spiEth(HSPI);

const char* Device_ID = "DEVICE001";

WiFiManager wm;
WiFiClient wifiClient;
EthernetClient ethClient;
PubSubClient client(wifiClient);

const char* mqtt_server = "evoluzn.org";
const int mqtt_port = 18889;
const char* mqtt_user = "evzin_led";
const char* mqtt_pass = "63I9YhMaXpa49Eb";
const char* mqtt_client = "Test_Mqtt_6776";

const char* AP_SSID = "DEVICE001";
const char* AP_PASS = "12345678";

bool apRunning = false;
bool ethernetReady = false;
bool modeInitialised = false;

unsigned long lastCheck = 0;
unsigned long mqttRetryAt = 0;  // ← Non-blocking MQTT retry

unsigned long lastLoRaSend = 0;
const unsigned long loraInterval = 2000;

static uint8_t ethMac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

enum class NetMode { WIFI,
                     ETHERNET };
static NetMode currentMode;

String offlineMsg = String(Device_ID) + ": offline";
String onlineMsg = String(Device_ID) + ": online";
String status_topic = "EthernetSystem/" + String(Device_ID) + "/status";
String subscribeTopic = "EthernetSystem/" + String(Device_ID) + "/#";

// ──────────────────────────────────────────────────────────────
//  Forward declarations
// ──────────────────────────────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectMQTT();

// ──────────────────────────────────────────────────────────────
//  MQTT - 5 attempts only
// ──────────────────────────────────────────────────────────────
void connectMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  int attempts = 0;

  while (!client.connected() && attempts < 5) {
    attempts++;
    Serial.printf("Connecting to MQTT... (attempt %d/5)\n", attempts);

    if (client.connect(mqtt_client, mqtt_user, mqtt_pass,
                       status_topic.c_str(), 0, true, offlineMsg.c_str())) {
      Serial.println("MQTT Connected");
      client.publish(status_topic.c_str(), onlineMsg.c_str(), true);
      client.subscribe(subscribeTopic.c_str());
    } else {
      Serial.printf("MQTT Failed rc=%d | Attempt %d/5\n", client.state(), attempts);
      delay(2000);
    }
  }

  if (!client.connected()) {
    Serial.println("MQTT: All 5 attempts failed. Will retry in 30s.");
    mqttRetryAt = millis() + 30000;  // 30 sec baad phir try karo
  }
}

// ──────────────────────────────────────────────────────────────
//  AP Start / Stop
// ──────────────────────────────────────────────────────────────
void startAP() {
  if (apRunning) return;

  Serial.println("[AP] Starting Config Portal...");
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(0);
  wm.startConfigPortal(AP_SSID, AP_PASS);

  apRunning = true;
}

// void startAP() {
//   if (apRunning) return;
//   Serial.println("Starting Config Portal...");
//   apRunning = true;
// }

void stopAP() {
  if (!apRunning) return;
  wm.stopConfigPortal();
  apRunning = false;
  Serial.println("AP Stopped");
}

static NetMode readButtonMode() {
  return (digitalRead(BUTTON_PIN) == LOW) ? NetMode::WIFI : NetMode::ETHERNET;
}

// ──────────────────────────────────────────────────────────────
//  Apply Mode
// ──────────────────────────────────────────────────────────────
static void applyMode(NetMode m) {

  if (client.connected()) {
    client.disconnect();
    delay(50);
  }

  currentMode = m;

  if (m == NetMode::WIFI) {
    Serial.println("[WiFi] Starting...");

    if (ethernetReady) {
      ethClient.stop();
      ethernetReady = false;
      delay(50);
    }

    WiFi.mode(WIFI_STA);

    // Pehle saved credentials se silently connect karne ki koshish karo (blocking, fast)
    // Agar credentials saved hain toh ~2-3 sec mein connect ho jaata hai
    WiFi.begin();  // Saved SSID + pass use karta hai
    Serial.print("[WiFi] Trying saved credentials");

    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 8000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
      apRunning = false;
      client.setClient(wifiClient);
      connectMQTT();

    } else {
      Serial.println("[WiFi] Saved credentials failed - opening portal: " + String(AP_SSID));
      wm.setConfigPortalBlocking(false);  // NON-BLOCKING - yahi main fix hai
      wm.setConfigPortalTimeout(0);       // Portal band nahi hoga khud se
      wm.startConfigPortal(AP_SSID, AP_PASS);
      apRunning = true;
      client.setClient(wifiClient);
    }

  } else {
    Serial.println("[ETH] Starting...");

    wm.stopConfigPortal();
    apRunning = false;
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);

    spiEth.begin(SPI_SCK, SPI_MISO, SPI_MOSI, ETH_CS_PIN);
    Ethernet.init(ETH_CS_PIN);

    if (Ethernet.begin(ethMac, 10000, 4000) == 0) {
      Serial.println("[ETH] DHCP failed - cable check karo");
      ethernetReady = false;
      return;
    }

    ethernetReady = true;
    Serial.printf("[ETH] Connected. IP: %s\n", Ethernet.localIP().toString().c_str());
    client.setClient(ethClient);
    connectMQTT();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

void wifiHealthCheck() {
  if (millis() - lastCheck < 5000) return;
  lastCheck = millis();
  Serial.printf("[WiFi] Status code: %d\n", WiFi.status());

  if (WiFi.status() == WL_CONNECTED) {
    if (apRunning) {
      Serial.println("[WiFi] Connected! Closing AP portal.");
      stopAP();
      // MQTT connect karo agar nahi hai
      if (!client.connected()) connectMQTT();
    }
    return;
  }

  Serial.println("[WiFi] Not connected - retrying saved credentials...");

  if (!apRunning) {
    Serial.println("[WiFi] Starting AP fallback...");
    startAP();  // 👈 yaha se AP start hoga
  }

  WiFi.reconnect();
}

// void wifiHealthCheck() {
//   if (millis() - lastCheck < 5000) return;
//   lastCheck = millis();

//   wl_status_t status = WiFi.status();
//   Serial.printf("[WiFi] Status: %d\n", status);

//   // ✅ If Connected
//   if (status == WL_CONNECTED) {

//     if (apRunning) {
//       Serial.println("[WiFi] Connected! Closing AP...");
//       stopAP();
//     }

//     if (!client.connected()) {
//       connectMQTT();
//     }

//     return;
//   }

//   // ❌ Not Connected
//   Serial.println("[WiFi] Not connected, trying saved credentials...");

//   WiFi.mode(WIFI_STA);
//   WiFi.begin();  // Try saved SSID

//   unsigned long startAttempt = millis();
//   while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println();

//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("[WiFi] Reconnect failed.");

//     // 🚀 Start AP if not already running
//     if (!apRunning) {
//       startAP();
//     }
//   } else {
//     Serial.println("[WiFi] Reconnected successfully!");
//   }
// }

void loraSendTask() {
  if (millis() - lastLoRaSend < loraInterval) return;
  lastLoRaSend = millis();

  String message = "T:23:01:31";

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket(true);  // FIX: true = async/non-blocking

  Serial.println("[LoRa] Sent: " + message);
}

// ──────────────────────────────────────────────────────────────
//  Setup
// ──────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Init Manager");

  //   wm.resetSettings();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  spiLoRa.begin(18, 19, 23, SS);
  LoRa.setSPI(spiLoRa);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {  // Change if needed
    Serial.println("LoRa Init Failed!");
    while (1)
      ;
  }

  currentMode = readButtonMode();
  applyMode(currentMode);
  modeInitialised = true;
}

// ──────────────────────────────────────────────────────────────
//  Loop
// ──────────────────────────────────────────────────────────────
void loop() {

  // 1. Button check
  NetMode desired = readButtonMode();
  if (modeInitialised && desired != currentMode) {
    Serial.printf("[MODE] %s -> %s\n",
                  currentMode == NetMode::WIFI ? "WiFi" : "Ethernet",
                  desired == NetMode::WIFI ? "WiFi" : "Ethernet");
    applyMode(desired);
  }

  // 2. WiFi tasks
  if (currentMode == NetMode::WIFI) {
    wm.process();       // Portal alive rakhta hai background mein
    wifiHealthCheck();  // Har 5s: saved WiFi try + portal band karo jab WiFi mile

    // MQTT reconnect - sirf agar connected hai aur timer allow kare
    if (WiFi.status() == WL_CONNECTED && !client.connected()
        && millis() > mqttRetryAt) {
      connectMQTT();
    }
    if (client.connected()) client.loop();
  }

  // 3. Ethernet tasks

  if (currentMode == NetMode::ETHERNET) {

    // Step 1: Check Ethernet cable status first
    bool ethernetConnected = (Ethernet.linkStatus() == LinkON);

    if (ethernetConnected) {

      // Step 2: Set ethernetReady true
      ethernetReady = true;

      // Step 3: Handle MQTT only if ethernet is ready
      if (ethernetReady) {

        // Try reconnecting MQTT if not connected
        if (!client.connected() && millis() > mqttRetryAt) {
          connectMQTT();
        }

        // Maintain MQTT connection
        if (client.connected()) {
          client.loop();
        }
      }
    }
  }

  loraSendTask();
}