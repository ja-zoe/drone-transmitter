#include "esp_err.h"

typedef enum {
  SW_OFF,
  SW_ON,
  SW_MID
} switch_state_t;

typedef enum {
  SW_ARM_DISARM,
  SW_SPDT_L,
  SW_SPDT_R,
  SW_SP3T_L,
  SW_SP3T_R,
} switch_id_t;

void get_switch_states(switch_state_t switch_states[5]);

esp_err_t configure_gpio_inputs();