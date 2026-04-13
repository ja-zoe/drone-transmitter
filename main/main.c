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

#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"
#include "i2cdev.h"


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
  ESP_ERROR_CHECK(ret);

  /*---- Initialize WiFi ----*/ 
  wifi_init();

  /*---- Initialize esp-now ----*/ 
  ESP_ERROR_CHECK( esp_now_init() );
  ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
  // ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

  /*---- Initialize I2C ----*/
  ESP_ERROR_CHECK(i2cdev_init());

  /*---- Configure GPIOs ----*/
    // Chose to configure GPIOS here instead of input read task because it should be guaranteed
    // that the gpios are configured before the 
  switch_gpio_config_t switch_gpio_config = {
    .PIN_SPDT_L = 2,
    .PIN_SPDT_R = 9,
    .PIN_SP3T_LH = 4,
    .PIN_SP3T_LL = 43,
    .PIN_SP3T_RH = 8,
    .PIN_SP3T_RL = 7,
    .PIN_ARM_DISARM = 44
  };
  configure_gpio_inputs(&switch_gpio_config);

  /*---- Initialize ADS1115 ----*/
  init_ads1115(5, 6, ADS111X_DATA_RATE_128);

  /*---- Add esp-now peers ----*/ 
  esp_now_peer_info_t peer_info = {
    .peer_addr = {0x8c, 0xbf, 0xea, 0x8e, 0x51, 0x78},
    .channel = 1,
    .ifidx = WIFI_IF_STA,
  };
  ret = esp_now_add_peer(&peer_info);
  ESP_ERROR_CHECK(ret);

  /*---- Create Shared Resources ----*/
  static control_packet_t control_packet;
  static telemetry_packet_t telemetry_packet;

  static SemaphoreHandle_t controlPacketMutexHandle;
  static StaticSemaphore_t controlPacketMutexBuffer;
  controlPacketMutexHandle = xSemaphoreCreateMutexStatic( &controlPacketMutexBuffer );
  configASSERT(controlPacketMutexHandle);
  
  static SemaphoreHandle_t telemetryPacketMutexHandle;
  static StaticSemaphore_t telemetryPacketMutexBuffer;
  telemetryPacketMutexHandle = xSemaphoreCreateMutexStatic( &telemetryPacketMutexBuffer );
  configASSERT(telemetryPacketMutexHandle);
  
  /*---- Start Tasks ----*/
  #define INPUT_TASK_STACK_SIZE 2048
  #define TRANSMIT_TASK_STACK_SIZE 4096
  #define RECEIVE_TASK_STACK_SIZE 4096
  #define OLED_TASK_STACK_SIZE 2048

    // Input Read Task
  static input_task_params_t input_task_params = {
    .control_packet = &control_packet,
  };
  input_task_params.lock = controlPacketMutexHandle;
  TaskHandle_t inputTaskHandle = NULL;
  static StaticTask_t inputTaskBuffer;
  static StackType_t inputTaskStack[ INPUT_TASK_STACK_SIZE ];
  inputTaskHandle = xTaskCreateStatic(
                inputReadTask,      /* Function that implements the task. */
                "Input-Read-Task",  /* Text name for the task. */
                INPUT_TASK_STACK_SIZE,         /* Number of indexes in the xStack array. */
                &input_task_params,       /* Parameter passed into the task. */
                tskIDLE_PRIORITY,   /* Priority at which the task is created. */
                inputTaskStack,     /* Array to use as the task's stack. */
                &inputTaskBuffer ); /* Variable to hold the task's data structure. */
  configASSERT(inputTaskHandle);
    
  // Data Transmit Task
  static transmit_task_params_t transmit_task_params = {
    .control_packet = &control_packet,
  };
  transmit_task_params.lock = controlPacketMutexHandle;
  transmit_task_params.des_addr = peer_info.peer_addr;
  TaskHandle_t transmitTaskHandle = NULL;
  static StaticTask_t transmitTaskBuffer;
  static StackType_t transmitTaskStack[ TRANSMIT_TASK_STACK_SIZE ];
  transmitTaskHandle = xTaskCreateStatic(
                dataTransmitTask,      /* Function that implements the task. */
                "Data-Transmit-Task",  /* Text name for the task. */
                TRANSMIT_TASK_STACK_SIZE,         /* Number of indexes in the xStack array. */
                &transmit_task_params,       /* Parameter passed into the task. */
                tskIDLE_PRIORITY,   /* Priority at which the task is created. */
                transmitTaskStack,     /* Array to use as the task's stack. */
                &transmitTaskBuffer ); /* Variable to hold the task's data structure. */
  configASSERT(transmitTaskHandle);
    
  // Data Receive Task
  static receive_task_params_t receive_task_params = {
    .telemetry_packet = &telemetry_packet,
  };
  receive_task_params.lock = telemetryPacketMutexHandle;
  receive_task_params.des_addr = peer_info.peer_addr;
  TaskHandle_t dataReceiveTaskHandle = NULL;
  static StaticTask_t dataReceiveTaskBuffer;
  static StackType_t dataReceiveTaskStack[ RECEIVE_TASK_STACK_SIZE ];
  dataReceiveTaskHandle = xTaskCreateStatic(
                dataReceiveTask,      /* Function that implements the task. */
                "Input-Read-Task",  /* Text name for the task. */
                RECEIVE_TASK_STACK_SIZE,         /* Number of indexes in the xStack array. */
                &receive_task_params,       /* Parameter passed into the task. */
                tskIDLE_PRIORITY,   /* Priority at which the task is created. */
                dataReceiveTaskStack,     /* Array to use as the task's stack. */
                &dataReceiveTaskBuffer ); /* Variable to hold the task's data structure. */
  configASSERT(dataReceiveTaskHandle);
    
  // OLED Update Task
  static oled_task_params_t oled_task_params = {
    .telemetry_packet = &telemetry_packet,
  };
  oled_task_params.lock = telemetryPacketMutexHandle;
  TaskHandle_t oledTaskHandle = NULL;
  static StaticTask_t oledTaskBuffer;
  static StackType_t oledTaskStack[ OLED_TASK_STACK_SIZE ];
  oledTaskHandle = xTaskCreateStatic(
                oledUpdateTask,      /* Function that implements the task. */
                "OLED-Update-Task",  /* Text name for the task. */
                OLED_TASK_STACK_SIZE,         /* Number of indexes in the xStack array. */
                &oled_task_params,       /* Parameter passed into the task. */
                tskIDLE_PRIORITY,   /* Priority at which the task is created. */
                oledTaskStack,     /* Array to use as the task's stack. */
                &oledTaskBuffer ); /* Variable to hold the task's data structure. */
  configASSERT(oledTaskHandle);
}