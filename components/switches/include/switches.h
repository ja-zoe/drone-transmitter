#pragma once

#include "esp_err.h"
#include "driver/gpio.h"

enum SwitchState {
  SW_OFF = 0,
  SW_ON  = 1,
  SW_MID = 2
};

typedef struct {
  gpio_num_t PIN_SPDT_L; 
  gpio_num_t PIN_SPDT_R; 
  gpio_num_t PIN_SP3T_LH;
  gpio_num_t PIN_SP3T_LL;
  gpio_num_t PIN_SP3T_RH;
  gpio_num_t PIN_SP3T_RL;
  gpio_num_t PIN_ARM_DISARM
} switch_gpio_config_t;

typedef struct {
  uint8_t arm_disarm;
  uint8_t spdt_l;
  uint8_t spdt_r;
  uint8_t sp3t_l;
  uint8_t sp3t_r;
} switch_states_t;

void get_switch_states(switch_states_t *switch_states);

void configure_gpio_inputs(const switch_gpio_config_t *switch_gpio_config);