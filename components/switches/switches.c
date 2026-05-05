#include <stdio.h>
#include "switches.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "config.h"

void configure_gpio_inputs() {
  // Configure pulled down gpio pins
  uint64_t pin_mask = (1ULL << PIN_SPDT_L_CONF) |
                      (1ULL << PIN_SPDT_R_CONF) |
                      (1ULL << PIN_SP3T_LH_CONF) |
                      (1ULL << PIN_SP3T_LL_CONF) |
                      (1ULL << PIN_SP3T_RH_CONF) |
                      (1ULL << PIN_SP3T_RL_CONF);

  gpio_config_t config_pulldown = {
    .pin_bit_mask = pin_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_down_en = GPIO_PULLDOWN_ENABLE
  };
  esp_err_t ret = gpio_config(&config_pulldown);
  ESP_ERROR_CHECK(ret);

  //Configure pulled up gpio pins
  pin_mask = (1ULL << PIN_ARM_DISARM_CONF);
  gpio_config_t config_pullup = {
    .pin_bit_mask = pin_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE
  };
  ret = gpio_config(&config_pullup);
  ESP_ERROR_CHECK(ret);

};

void get_switch_states(switch_states_t *switch_states) {
  // Arm Disarm (Index 0) 
  switch_states->arm_disarm = gpio_get_level(PIN_ARM_DISARM_CONF) ? SW_ON : SW_OFF;
  
  // Left SPDT (Index 1)
  switch_states->spdt_l = gpio_get_level(PIN_SPDT_L_CONF) ? SW_ON : SW_OFF;
  
  // Right SPDT (Index 2)
  switch_states->spdt_r = gpio_get_level(PIN_SPDT_R_CONF) ? SW_ON : SW_OFF;
  
  // Left SP3T (Index 3) - 3-position switch
  int lh = gpio_get_level(PIN_SP3T_LH_CONF);
  int ll = gpio_get_level(PIN_SP3T_LL_CONF);
  if (lh && !ll) {
      switch_states->sp3t_l = SW_ON;      // High position
  } else if (!lh && ll) {
      switch_states->sp3t_l = SW_OFF;     // Low position
  } else {
      switch_states->sp3t_l = SW_MID;     // Middle position
  }
  
  // Right SP3T (Index 4) - 3-position switch
  int rh = gpio_get_level(PIN_SP3T_RH_CONF);
  int rl = gpio_get_level(PIN_SP3T_RL_CONF);
  if (rh && !rl) {
      switch_states->sp3t_r = SW_ON;      // High position
  } else if (!rh && rl) {
      switch_states->sp3t_r = SW_OFF;     // Low position
  } else {
      switch_states->sp3t_r = SW_MID;     // Middle position
  }
};