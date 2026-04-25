#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "drone_common.h"

enum SwitchState {
  SW_OFF = 0,
  SW_ON  = 1,
  SW_MID = 2
};

void get_switch_states(switch_states_t *switch_states);

void configure_gpio_inputs(void);