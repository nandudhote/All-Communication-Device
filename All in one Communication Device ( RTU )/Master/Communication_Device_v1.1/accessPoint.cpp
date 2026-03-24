#include "accessPoint.h"
#include "Config.h"
#include "myWiFi.h"
#include <WiFi.h>
#include "internalDrivers.h"
#include <AsyncTCP.h>  // Use ESPAsyncTCP for ESP8266
#include <ESPAsyncWebServer.h>

myWIFI wifi;
internalDrivers iiiDrivers;
subPubTopics iTopic;
AsyncWebServer serverW(80);

ACCESSPOINT::ACCESSPOINT() {}

void ACCESSPOINT::accessPointSetup() {
  if (((SSID == "") && (PASSWORD == "")) || ((ssidLength == 255) || (passwordLength == 255))) {
    readSsidAndPasswordFromAP(apSSID.c_str(), apPassword.c_str());
    //Serial.print("Waiting for SSID and Password from AP");
    while (((SSID == "") && (PASSWORD == "")) || ((ssidLength == 255) || (passwordLength == 255))) {
      digitalWrite(_wifiStatusLED, HIGH);
      delay(100);
      digitalWrite(_wifiStatusLED, LOW);
      delay(100);
      //Serial.print(".");
    }
    //Serial.println("Received SSID and Password");
    wifi.wiFiSetup(SSID, PASSWORD);
  } else {
    wifi.wiFiSetup(SSID, PASSWORD);
  }
}

void ACCESSPOINT::updateTheSSIDAndPASSFromMqttIfAvailable() {
  if (globallySSIDAndPasswordChange) {
    bool wiFiStatus = wifi.wiFiSetup(SSID, PASSWORD);
    //Serial.println(SSID);
    //Serial.println(PASSWORD);
    wifi.reconnectToMqtt(iTopic.Publish, iTopic.Subscribe, iTopic.Global, iTopic.Alert);
    if (wiFiStatus) {
      wifi.publishMqttMsg_Alert(iTopic.Publish, deviceId, "Wi-Fi Credentials Changed");
      iiiDrivers.writeOneByteInEEPROM(ssidLenghtEEPROMAdd, SSID.length());
      iiiDrivers.writeOneByteInEEPROM(passwordLenghtEEPROMAdd, PASSWORD.length());
      iiiDrivers.storeStringInEEPROM(SSID, ssidEEPROMAdd);
      iiiDrivers.storeStringInEEPROM(PASSWORD, passEEPROMAdd);
    } else {
      wifi.publishMqttMsg_Alert(iTopic.Publish, deviceId, "Wi-Fi Credentials not Changed");
    }
    globallySSIDAndPasswordChange = false;
  }
}

void ACCESSPOINT::readSsidAndPasswordFromAP(const char* ssid, const char* pass) {
  WiFi.softAP(ssid, pass);
  //Serial.println("Access Point SSID: " + String(ssid));
  //Serial.println("Access Point Password: " + String(pass));

  serverW.on("/wiFiCredentials", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ssidAndPass")) {
      const AsyncWebParameter* param = request->getParam("ssidAndPass");
      if (param != nullptr) {
        String SSIDANDPASS = param->value();
        //Serial.print("Received Data: ");
        //Serial.println(SSIDANDPASS);
        request->send(200, "text/plain", deviceId);

        int delimiterIndex = SSIDANDPASS.indexOf(':');
        SSID = SSIDANDPASS.substring(0, delimiterIndex);
        PASSWORD = SSIDANDPASS.substring(delimiterIndex + 1);

        //Serial.print("tempSSID: ");
        //Serial.println(SSID);
        //Serial.print("tempPASSWORD: ");
        //Serial.println(PASSWORD);

        wifi.earasWiFiCredentialsFromEEPROM();
        //Serial.println("After Clearing EEPROM");
        iiiDrivers.writeOneByteInEEPROM(ssidLenghtEEPROMAdd, SSID.length());
        iiiDrivers.writeOneByteInEEPROM(passwordLenghtEEPROMAdd, PASSWORD.length());
        //Serial.println("After Writing EEPROM");
        iiiDrivers.storeStringInEEPROM(SSID, ssidEEPROMAdd);
        iiiDrivers.storeStringInEEPROM(PASSWORD, passEEPROMAdd);
        //Serial.println("After Storing EEPROM");
      } else {
        request->send(400);  // Bad Request
      }
    } else {
      request->send(400);  // Bad Request
    }
  });
  serverW.begin();
}

void ACCESSPOINT::stopApServer() {
  serverW.end();
  //Serial.println("Server stopped.");
}

void ACCESSPOINT::stopApWiFi() {
  WiFi.softAPdisconnect(true);
  //Serial.println("Access Point mode stopped.");
}
