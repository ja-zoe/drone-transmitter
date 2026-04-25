#include <stdio.h>
#include "joysticks.h"
#include "ads1115.h"
#include "esp_err.h"
#include "setup.h"
#include "drone_common.h"

ads1115_t ads;
extern i2c_master_bus_handle_t i2c_bus;
joystick_cal_t joysticks_cal;

void init_ads1115()
{  
  ESP_ERROR_CHECK(ads1115_init(&ads, &i2c_bus, ADS_I2C_ADDR_GND, ADS_I2C_FREQ_CONF));

  ads1115_set_gain(&ads, ADS_FSR_4_096V);
  ads1115_set_sps(&ads, ADS_SPS_128);
}


void get_joysticks_raw(joysticks_values_t *values) 
{  
  values->throttle = ads1115_get_raw(&ads, THROTTLE_MUX_CONF);
  values->roll = ads1115_get_raw(&ads, ROLL_MUX_CONF);
  values->pitch = ads1115_get_raw(&ads, PITCH_MUX_CONF);
  values->yaw = ads1115_get_raw(&ads, YAW_MUX_CONF);
}

int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void get_joysticks_calibrated(joysticks_values_t *values) {
    joysticks_values_t raw;
    get_joysticks_raw(&raw);

    // Map Throttle: Raw Range [min, max] -> Output Range [STICK_OUT_MIN, STICK_OUT_MAX]
    values->throttle = map(raw.throttle, 
                           joysticks_cal.throttle.min, joysticks_cal.throttle.max, 
                           STICK_OUT_MIN, STICK_OUT_MAX);

    // Map Yaw
    values->yaw = map(raw.yaw, 
                      joysticks_cal.yaw.min, joysticks_cal.yaw.max, 
                      STICK_OUT_MIN, STICK_OUT_MAX);

    // Map Pitch
    values->pitch = map(raw.pitch, 
                        joysticks_cal.pitch.min, joysticks_cal.pitch.max, 
                        STICK_OUT_MIN, STICK_OUT_MAX);

    // Map Roll
    values->roll = map(raw.roll, 
                       joysticks_cal.roll.min, joysticks_cal.roll.max, 
                       STICK_OUT_MIN, STICK_OUT_MAX);

    // --- Safety Clamping ---
    // This ensures no values exceed your defined Macro bounds
    if (values->throttle > STICK_OUT_MAX) values->throttle = STICK_OUT_MAX;
    if (values->throttle < STICK_OUT_MIN) values->throttle = STICK_OUT_MIN;
    
    if (values->yaw > STICK_OUT_MAX) values->yaw = STICK_OUT_MAX;
    if (values->yaw < STICK_OUT_MIN) values->yaw = STICK_OUT_MIN;

    if (values->pitch > STICK_OUT_MAX) values->pitch = STICK_OUT_MAX;
    if (values->pitch < STICK_OUT_MIN) values->pitch = STICK_OUT_MIN;

    if (values->roll > STICK_OUT_MAX) values->roll = STICK_OUT_MAX;
    if (values->roll < STICK_OUT_MIN) values->roll = STICK_OUT_MIN;
}

void set_calibration_values(joystick_cal_t *cal) {
  joysticks_cal = *cal;
}