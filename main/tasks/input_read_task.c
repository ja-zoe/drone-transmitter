#include "freertos/FreeRTOS.h"
#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"
#include "esp_log.h"

static const char *TAG = "Input_Read_Task";

void inputReadTask( void *pvParameters) {
  static uint32_t lock_fail_count;
  static control_packet_t control_packet;

  input_task_params_t *data = (input_task_params_t *) pvParameters;

  if (data->lock == NULL) {
    configASSERT(data->lock);
  }

  while (1) {
    get_switch_states(&control_packet.switches_values);
    get_joysticks_values(&control_packet.joysticks_values);
  
    if (xSemaphoreTake(data->lock, pdMS_TO_TICKS(3)) == pdFALSE) {
      lock_fail_count++;
      ESP_LOGI(TAG, "Failed to take mutex on attempt %u", lock_fail_count);
      continue;
    }

    *(data->control_packet) = control_packet;
    xSemaphoreGive(data->lock);
  }
};