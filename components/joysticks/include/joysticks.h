#pragma once
#include "ads111x.h"

typedef struct {
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
} joysticks_values_t;

void init_ads1115(gpio_num_t sda_gpio, gpio_num_t scl_gpio, ads111x_data_rate_t rate);

void get_joysticks_values(joysticks_values_t *values);