#pragma once
#include "drone_common.h"
#include "freertos/FreeRTOS.h"

/* ESPNOW Config */
#define ESPNOW_PEER_ADDR_CONF {0x8c, 0xbf, 0xea, 0x8e, 0x51, 0x78}

/* Switches Config */
#define PIN_SPDT_L_CONF 2
#define PIN_SPDT_R_CONF 9
#define PIN_SP3T_LH_CONF 4
#define PIN_SP3T_LL_CONF 43
#define PIN_SP3T_RH_CONF 8
#define PIN_SP3T_RL_CONF 7
#define PIN_ARM_DISARM_CONF 44

/* I2C Config */
#define I2C_PORT_CONF 0
#define SDA_GPIO_CONF 5
#define SCL_GPIO_CONF 6

/* OLED Config */
#define OLED_FREQ_CONF (1000 * 1000)
#define OLED_I2C_ADDR_CONF 0x3D
#define OLED_H_RES_CONF 128
#define OLED_V_RES_CONF 64

/* ADS115 (Joystick ADC) Config */
#define ADS_I2C_FREQ_CONF (400 * 1000)
#define THROTTLE_MUX_CONF 0
#define YAW_MUX_CONF 1
#define PITCH_MUX_CONF 2
#define ROLL_MUX_CONF 3

/* Task Config */
#define INPUT_TASK_STACK_SIZE 2048
#define TRANSMIT_TASK_STACK_SIZE 4096
#define RECEIVE_TASK_STACK_SIZE 4096
#define OLED_TASK_STACK_SIZE 2048

void nvs_init(void);
void wifi_init(void);
void oled_init(void);
void i2c_init(void);
void calibrate_joysticks(void);
void start_calibration_sequence(void);