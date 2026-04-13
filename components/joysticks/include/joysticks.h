#pragma once
#include "ads111x.h"
#include "drone_common.h"

void init_ads1115(gpio_num_t sda_gpio, gpio_num_t scl_gpio, ads111x_data_rate_t rate);

void get_joysticks_values(joysticks_values_t *values);