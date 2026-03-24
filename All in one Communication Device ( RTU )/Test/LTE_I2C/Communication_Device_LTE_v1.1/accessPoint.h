#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

class ACCESSPOINT {
public:
  ACCESSPOINT();
  void accessPointSetup();
  void updateTheSSIDAndPASSFromMqttIfAvailable();
  void readSsidAndPasswordFromAP(const char* ssid, const char* pass);
  void stopApServer();
  void stopApWiFi();
};

#endif