#pragma once
#include "drone_common.h"

typedef struct {
    int16_t min;
    int16_t max;
} axis_cal_t;

typedef struct {
    axis_cal_t throttle;
    axis_cal_t yaw;
    axis_cal_t pitch;
    axis_cal_t roll;
} joystick_cal_t;

void init_ads1115(void);

void get_joysticks_raw(joysticks_values_t *values);

void get_joysticks_calibrated(joysticks_values_t *values);

void set_calibration_values(joystick_cal_t *cal);