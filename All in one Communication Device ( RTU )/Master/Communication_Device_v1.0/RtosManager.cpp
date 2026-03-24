#include "RtosManager.h"  // Header for RtosManager class (task & queue handling)
#include "Config.h"
#include "myWIFI.h"
// #include "w5500.h"
#include "loraEsp.h"
#include "i2c.h"
#include "internalDrivers.h"



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
  while (true) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void RtosManager::txTaskLoop() {

  while (true) {

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
