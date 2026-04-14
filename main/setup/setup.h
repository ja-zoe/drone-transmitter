#pragma once

#define ESPNOW_PEER_ADDR_CONF {0x8c, 0xbf, 0xea, 0x8e, 0x51, 0x78}

#define PIN_SPDT_L_CONF 2
#define PIN_SPDT_R_CONF 9
#define PIN_SP3T_LH_CONF 4
#define PIN_SP3T_LL_CONF 43
#define PIN_SP3T_RH_CONF 8
#define PIN_SP3T_RL_CONF 7
#define PIN_ARM_DISARM_CONF 44

#define SDA_GPIO_CONF 5
#define SCL_GPIO_CONF 6

#define INPUT_TASK_STACK_SIZE 2048
#define TRANSMIT_TASK_STACK_SIZE 4096
#define RECEIVE_TASK_STACK_SIZE 4096
#define OLED_TASK_STACK_SIZE 2048

void nvs_init(void);

void wifi_init(void);
