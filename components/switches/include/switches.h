typedef enum {
  SW_ACTIVATED,
  SW_DEACTIVATED,
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

switch_state_t get_switch_state(switch_id_t switch_id);