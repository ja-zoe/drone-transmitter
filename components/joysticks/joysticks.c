#include <stdio.h>
#include "joysticks.h"
#include "ads1115.h"
#include "esp_err.h"
#include "setup.h"


ads1115_t ads;
extern i2c_master_bus_handle_t i2c_bus;

void init_ads1115()
{  
  ESP_ERROR_CHECK(ads1115_init(&ads, &i2c_bus, ADS_I2C_ADDR_GND, ADS_I2C_FREQ_CONF));

  ads1115_set_gain(&ads, ADS_FSR_4_096V);
  ads1115_set_sps(&ads, ADS_SPS_128);
}


void get_joysticks_values(joysticks_values_t *values) 
{  
  values->throttle = ads1115_get_raw(&ads, 0);
  values->roll = ads1115_get_raw(&ads, 1);
  values->pitch = ads1115_get_raw(&ads, 2);
  values->yaw = ads1115_get_raw(&ads, 3);
}