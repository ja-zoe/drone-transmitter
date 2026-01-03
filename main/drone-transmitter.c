#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#define LED_PIN GPIO_NUM_7

gpio_config_t led_gpio_config = {
  .pin_bit_mask = 1ULL << LED_PIN,
  .mode = GPIO_MODE_OUTPUT
};

void app_main(void)
{
  gpio_config(&led_gpio_config);
  while(1) {
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  };
}