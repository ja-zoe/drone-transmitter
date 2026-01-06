#include "esp_err.h"

enum SwitchState {
  SW_OFF = 0,
  SW_ON  = 1,
  SW_MID = 2
};

enum switch_id_t {
  SW_ARM_DISARM = 0,
  SW_SPDT_L     = 1,
  SW_SPDT_R     = 2,
  SW_SP3T_L     = 3,
  SW_SP3T_R     = 4
};

void get_switch_states(uint8_t switch_states[5]);

esp_err_t configure_gpio_inputs();