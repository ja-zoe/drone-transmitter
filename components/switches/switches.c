#include <stdio.h>
#include "switches.h"
#include "driver/gpio.h"
#include "esp_err.h"

static switch_gpio_config_t switch_gpio_config_global;

void configure_gpio_inputs(const switch_gpio_config_t *switch_gpio_config) {
  switch_gpio_config_global.PIN_SPDT_L = switch_gpio_config->PIN_SPDT_L;
  switch_gpio_config_global.PIN_SPDT_R = switch_gpio_config->PIN_SPDT_R;
  switch_gpio_config_global.PIN_SP3T_LH = switch_gpio_config->PIN_SP3T_LH;
  switch_gpio_config_global.PIN_SP3T_LL = switch_gpio_config->PIN_SP3T_LL;
  switch_gpio_config_global.PIN_SP3T_RH = switch_gpio_config->PIN_SP3T_RH;
  switch_gpio_config_global.PIN_SP3T_RL = switch_gpio_config->PIN_SP3T_RL;
  switch_gpio_config_global.PIN_ARM_DISARM = switch_gpio_config->PIN_ARM_DISARM;

  uint64_t pin_mask = (1ULL << switch_gpio_config_global.PIN_SPDT_L) |
                      (1ULL << switch_gpio_config_global.PIN_SPDT_R) |
                      (1ULL << switch_gpio_config_global.PIN_SP3T_LH) |
                      (1ULL << switch_gpio_config_global.PIN_SP3T_LL) |
                      (1ULL << switch_gpio_config_global.PIN_SP3T_RH) |
                      (1ULL << switch_gpio_config_global.PIN_SP3T_RL) |
                      (1ULL << switch_gpio_config_global.PIN_ARM_DISARM);

  gpio_config_t config = {
    .pin_bit_mask = pin_mask,
    .mode = GPIO_MODE_INPUT,
    .pull_down_en = GPIO_PULLDOWN_ENABLE
  };

  esp_err_t esp_ret = gpio_config(&config);
  ESP_ERROR_CHECK(esp_ret);
};

void get_switch_states(switch_states_t *switch_states) {
  // Arm Disarm (Index 0) 
  switch_states->arm_disarm = gpio_get_level(switch_gpio_config_global.PIN_ARM_DISARM) ? SW_ON : SW_OFF;
  
  // Left SPDT (Index 1)
  switch_states->spdt_l = gpio_get_level(switch_gpio_config_global.PIN_SPDT_L) ? SW_ON : SW_OFF;
  
  // Right SPDT (Index 2)
  switch_states->spdt_r = gpio_get_level(switch_gpio_config_global.PIN_SPDT_R) ? SW_ON : SW_OFF;
  
  // Left SP3T (Index 3) - 3-position switch
  int lh = gpio_get_level(switch_gpio_config_global.PIN_SP3T_LH);
  int ll = gpio_get_level(switch_gpio_config_global.PIN_SP3T_LL);
  if (lh && !ll) {
      switch_states->sp3t_l = SW_ON;      // High position
  } else if (!lh && ll) {
      switch_states->sp3t_l = SW_OFF;     // Low position
  } else {
      switch_states->sp3t_l = SW_MID;     // Middle position
  }
  
  // Right SP3T (Index 4) - 3-position switch
  int rh = gpio_get_level(switch_gpio_config_global.PIN_SP3T_RH);
  int rl = gpio_get_level(switch_gpio_config_global.PIN_SP3T_RL);
  if (rh && !rl) {
      switch_states->sp3t_r = SW_ON;      // High position
  } else if (!rh && rl) {
      switch_states->sp3t_r = SW_OFF;     // Low position
  } else {
      switch_states->sp3t_r = SW_MID;     // Middle position
  }
};