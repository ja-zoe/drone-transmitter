#include "freertos/FreeRTOS.h"
#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"
#include "esp_log.h"
#include "esp_now.h"
#include "config.h"

static const char *TAG = "Transmit_Task";
extern control_packet_t control_packet;
extern SemaphoreHandle_t controlPacketMutexHandle;

void dataTransmitTask( void *pvParameters ) {
  esp_err_t ret;

  static const uint8_t peer_addr[6] = ESPNOW_PEER_ADDR_CONF;
  static uint32_t lock_fail_count;
  static control_packet_t control_packet_in;

  configASSERT(controlPacketMutexHandle);
  
  while(1){
    if (xSemaphoreTake(controlPacketMutexHandle, pdMS_TO_TICKS(3)) == pdFALSE) {
      lock_fail_count++;
      ESP_LOGI(TAG, "Failed to take mutex on attempt %u", lock_fail_count);
      continue;
    }
    control_packet_in = control_packet; // Copy global control packet into internal one
    xSemaphoreGive(controlPacketMutexHandle);

    ret = esp_now_send(peer_addr, (uint8_t *) &control_packet_in, sizeof(control_packet_in));
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Error transmitting control data: %s", esp_err_to_name(ret));
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  };
};