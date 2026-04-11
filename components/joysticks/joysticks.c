#include <stdio.h>
#include "joysticks.h"
#include "ads111x.h"
#include "esp_err.h"

esp_err_t ret;

i2c_dev_t dev;

void init_ads1115(gpio_num_t sda_gpio, gpio_num_t scl_gpio, ads111x_data_rate_t rate)
{
  ret = ads111x_init_desc(
    &dev, 
    ADS111X_ADDR_GND,  
    I2C_NUM_0,
    sda_gpio, 
    scl_gpio);
  ESP_ERROR_CHECK(ret);

  ret = ads111x_set_data_rate(&dev, rate); 
  ESP_ERROR_CHECK(ret);

  ret = ads111x_set_mode(&dev, ADS111X_MODE_CONTINUOUS);
  ESP_ERROR_CHECK(ret);
}


void get_joysticks_values(joysticks_values_t *values) 
{  
  ads111x_set_input_mux(&dev, ADS111X_MUX_0_GND);
  vTaskDelay(pdMS_TO_TICKS(25));
  ads111x_get_value(&dev, &values->ry);

  ads111x_set_input_mux(&dev, ADS111X_MUX_1_GND);
  vTaskDelay(pdMS_TO_TICKS(25));
  ads111x_get_value(&dev, &values->rx);

  ads111x_set_input_mux(&dev, ADS111X_MUX_2_GND);
  vTaskDelay(pdMS_TO_TICKS(25));
  ads111x_get_value(&dev, &values->lx);

  ads111x_set_input_mux(&dev, ADS111X_MUX_3_GND);
  vTaskDelay(pdMS_TO_TICKS(25));
  ads111x_get_value(&dev, &values->ly);
}