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
    get_joysticks_calibrated(&control_packet.joysticks_values);
    ESP_LOGI(TAG, 
      "Sticks -> T: %4d | Y: %4d | P: %4d | R: %4d  "
      "Switches -> Arm: %d | L_2T: %d | R_2T: %d | L_3T: %d | R_3T: %d",
      control_packet.joysticks_values.throttle,
      control_packet.joysticks_values.yaw,
      control_packet.joysticks_values.pitch,
      control_packet.joysticks_values.roll,
      control_packet.switches_values.arm_disarm,
      control_packet.switches_values.spdt_l,
      control_packet.switches_values.spdt_r,
      control_packet.switches_values.sp3t_l,
      control_packet.switches_values.sp3t_r
    );
    if (xSemaphoreTake(data->lock, pdMS_TO_TICKS(3)) == pdFALSE) {
      lock_fail_count++;
      ESP_LOGI(TAG, "Failed to take mutex on attempt %u", lock_fail_count);
      continue;
    }

    *(data->control_packet) = control_packet;
    xSemaphoreGive(data->lock);
    vTaskDelay(pdMS_TO_TICKS(60));
  }
};