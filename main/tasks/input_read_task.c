#include "freertos/FreeRTOS.h"
#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"
#include "esp_log.h"

static const char *TAG = "Input_Read_Task";
extern control_packet_t control_packet;
extern SemaphoreHandle_t controlPacketMutexHandle;

void inputReadTask( void *pvParameters) {
  static uint32_t lock_fail_count;
  control_packet_t control_packet_in;
  
  configASSERT(controlPacketMutexHandle);
  
  while (1) {
    get_switch_states(&control_packet_in.switches_values);
    get_joysticks_calibrated(&control_packet_in.joysticks_values);
    ESP_LOGI(TAG, 
      "Sticks -> T: %4d | Y: %4d | P: %4d | R: %4d"
      "Switches -> Arm: %d | L_2T: %d | R_2T: %d | L_3T: %d | R_3T: %d",
      control_packet_in.joysticks_values.throttle,
      control_packet_in.joysticks_values.yaw,
      control_packet_in.joysticks_values.pitch,
      control_packet_in.joysticks_values.roll,
      control_packet_in.switches_values.arm_disarm,
      control_packet_in.switches_values.spdt_l,
      control_packet_in.switches_values.spdt_r,
      control_packet_in.switches_values.sp3t_l,
      control_packet_in.switches_values.sp3t_r
    );
    if (xSemaphoreTake(controlPacketMutexHandle, pdMS_TO_TICKS(3)) == pdFALSE) {
      lock_fail_count++;
      ESP_LOGI(TAG, "Failed to take mutex on attempt %u", lock_fail_count);
      continue;
    }

    control_packet = control_packet_in;
    xSemaphoreGive(controlPacketMutexHandle);
    vTaskDelay(pdMS_TO_TICKS(60));
  }
};