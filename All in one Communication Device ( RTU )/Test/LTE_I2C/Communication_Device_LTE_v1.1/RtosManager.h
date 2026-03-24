#ifndef RTOS_MANAGER_H
#define RTOS_MANAGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

class RtosManager {
public:
  RtosManager();
  void begin();

private:
  // RTOS handles
  TaskHandle_t rxTaskHandle;
  TaskHandle_t txTaskHandle;
  QueueHandle_t rxQueue;

  // Task entry points (must be static)
  static void rxTask(void *pvParameters);
  static void txTask(void *pvParameters);

  // Real task logic
  void rxTaskLoop();
  void txTaskLoop();
  void processLine(String line);
  void appendToPrefs(const String &data);
  String buildJsonFromPrefs();
  void clearPrefs();
  bool hasPendingData();
};

#endif
