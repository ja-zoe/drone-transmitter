#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "switches.h"
#include "nvs_flash.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

static void espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_FAIL) {
    printf("ESP now delivery failed\n");
  } else {
    printf("ESP now delivery sucecess\n");
  }
};

void app_main(void)
{
  esp_err_t ret;

  /*---- Initialize NVS for persistent wifi credentials ----*/ 
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK( nvs_flash_erase() );
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  /*---- Initialize WiFi ----*/ 
  wifi_init();

  /*---- Initialize esp-now ----*/ 
  ESP_ERROR_CHECK( esp_now_init() );
  ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
  // ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

  /*---- Add esp-now peers ----*/ 
  esp_now_peer_info_t peer_info = {
    .peer_addr = {0x8c, 0xbf, 0xea, 0x8e, 0x51, 0x78},
    .channel = 1,
    .ifidx = WIFI_IF_STA,
  };
  ret = esp_now_add_peer(&peer_info);
  ESP_ERROR_CHECK(ret);

  /*----  Configure switch gpios as internal pull down inputs----*/
  ret = configure_gpio_inputs();
  ESP_ERROR_CHECK(ret);

  uint8_t switch_states[5] = {0};
  while(1) {
    get_switch_states(switch_states);
    esp_now_send(NULL, switch_states, sizeof(switch_states));
    vTaskDelay(pdMS_TO_TICKS(200));
  };
}