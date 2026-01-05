#include <stdio.h>
#include "switches.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define PIN_SPDT_L 43
#define PIN_SPDT_R 2
#define PIN_SP3T_LH 44
#define PIN_SP3T_LL 7
#define PIN_SP3T_RH 3
#define PIN_SP3T_RL 4
#define PIN_ARM_DISARM 1

esp_err_t configure_gpio_inputs() {
  uint64_t pin_mask = (1ULL << PIN_SPDT_L) |
                      (1ULL << PIN_SPDT_R) |
                      (1ULL << PIN_SP3T_LH) |
                      (1ULL << PIN_SP3T_LL) |
                      (1ULL << PIN_SP3T_RH) |
                      (1ULL << PIN_SP3T_RL) |
                      (1ULL << PIN_ARM_DISARM);

  gpio_config_t config = {
    .pin_bit_mask = pin_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_down_en = GPIO_PULLDOWN_ENABLE
  };

  esp_err_t esp_ret = gpio_config(&config);
  ESP_ERROR_CHECK(esp_ret);
  return ESP_OK;
};

void get_switch_states(switch_state_t switch_states[5]) {
  // Arm Disarm (Index 0) 
  switch_states[SW_ARM_DISARM] = gpio_get_level(PIN_ARM_DISARM) ? SW_ON : SW_OFF;
  
  // Left SPDT (Index 1)
  switch_states[SW_SPDT_L] = gpio_get_level(PIN_SPDT_L) ? SW_ON : SW_OFF;
  
  // Right SPDT (Index 2)
  switch_states[SW_SPDT_R] = gpio_get_level(PIN_SPDT_R) ? SW_ON : SW_OFF;
  
  // Left SP3T (Index 3) - 3-position switch
  int lh = gpio_get_level(PIN_SP3T_LH);
  int ll = gpio_get_level(PIN_SP3T_LL);
  if (lh && !ll) {
      switch_states[SW_SP3T_L] = SW_ON;      // High position
  } else if (!lh && ll) {
      switch_states[SW_SP3T_L] = SW_OFF;     // Low position
  } else {
      switch_states[SW_SP3T_L] = SW_MID;     // Middle position
  }
  
  // Right SP3T (Index 4) - 3-position switch
  int rh = gpio_get_level(PIN_SP3T_RH);
  int rl = gpio_get_level(PIN_SP3T_RL);
  if (rh && !rl) {
      switch_states[SW_SP3T_R] = SW_ON;      // High position
  } else if (!rh && rl) {
      switch_states[SW_SP3T_R] = SW_OFF;     // Low position
  } else {
      switch_states[SW_SP3T_R] = SW_MID;     // Middle position
  }
};