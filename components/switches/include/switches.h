typedef enum {
  SW_OFF,
  SW_ON,
  SW_MID
} switch_state_t;

typedef enum {
  SW_SPDT_L,
  SW_SPDT_R,
  SW_SP3T_L,
  SW_SP3T_R,
  SW_ARM_DISARM,
  SW_POWER
} switch_id_t;

esp_err_t get_switch_states(switch_state_t switch_states[5]);