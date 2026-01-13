#pragma once

typedef struct {
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
} joysticks_values_t;

void init_ads1115(void);

void get_joysticks_values(joysticks_values_t *values);