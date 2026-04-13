#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"
#include "esp_log.h"
#include "esp_now.h"

static const char *TAG = "Transmit_Task";

void dataTransmitTask( void *pvParameters ) {
  esp_err_t ret;

  static uint32_t lock_fail_count;
  static control_packet_t control_packet;

  transmit_task_params_t *data = (transmit_task_params_t *) pvParameters;

  if (data->lock == NULL) {
    configASSERT(data->lock);
  }

  while(1){
    if (xSemaphoreTake(data->lock, pdMS_TO_TICKS(3)) == pdFALSE) {
      lock_fail_count++;
      continue;
    }

    control_packet = *(data->control_packet);
    xSemaphoreGive(data->lock);

    ret = esp_now_send(data->des_addr, (uint8_t *) &control_packet, sizeof(control_packet));
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Error transmitting control data: %s", esp_err_to_name(ret));
    }
  };
};