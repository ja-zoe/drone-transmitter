#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"
#include "switches.h"
#include "setup.h"
#include "espnow.h"
#include "calibration.h"

static char *TAG = "main";

// Shared controls packet
control_packet_t control_packet;
SemaphoreHandle_t controlPacketMutexHandle;

// Shared telemetry packet
telemetry_packet_t telemetry_packet;
SemaphoreHandle_t telemetryPacketMutexHandle;

void app_main(void)
{  
/*---- Initialize nvs memory for persistent wifi credentials ----*/ 
  ESP_LOGI(TAG, "Initializing NVS memory...");
  nvs_init();
  ESP_LOGI(TAG, "Done initializing NVS memory");
/*---- Initialize WiFi ----*/ 
  ESP_LOGI(TAG, "Initializing WIFI...");  
  wifi_init();
  ESP_LOGI(TAG, "Done initializing WIFI");  
/*---- Initialize ESPNOW ----*/ 
  ESP_LOGI(TAG, "Initializing ESPNOW...");  
  init_espnow();
  ESP_LOGI(TAG, "Done initializing ESPNOW");  
/*---- Initialize I2C ----*/
  ESP_LOGI(TAG, "Initializing I2C...");  
  i2c_init();
  ESP_LOGI(TAG, "Done initializing I2C");  
/*---- Initialize OLED ----*/
  ESP_LOGI(TAG, "Initializing OLED...");  
  oled_init();
  ESP_LOGI(TAG, "Done initializing OLED");  
/*---- Configure GPIOs ----*/
  ESP_LOGI(TAG, "Initializing GPIOs...");  
  configure_gpio_inputs();
  ESP_LOGI(TAG, "Done initializing GPIOs");  
/*---- Initialize ADS1115 ----*/
  ESP_LOGI(TAG, "Initializing ADC...");    
  init_ads1115();
  ESP_LOGI(TAG, "Done initializing ADC");   
/*---- Start joystick calibration ----*/
  ESP_LOGI(TAG, "Initializing joystick calibration...");    
  start_calibration_sequence();
  ESP_LOGI(TAG, "Done initializing joystick calibration");  

/*---- Create Shared Resources ----*/
  static StaticSemaphore_t controlPacketMutexBuffer;
  controlPacketMutexHandle = xSemaphoreCreateMutexStatic( &controlPacketMutexBuffer );
  configASSERT(controlPacketMutexHandle);
  
  static StaticSemaphore_t telemetryPacketMutexBuffer;
  telemetryPacketMutexHandle = xSemaphoreCreateMutexStatic( &telemetryPacketMutexBuffer );
  configASSERT(telemetryPacketMutexHandle);

/*---- Start Tasks ----*/
  // Input Read Task
  TaskHandle_t inputTaskHandle = NULL;
  static StaticTask_t inputTaskBuffer;
  static StackType_t inputTaskStack[ INPUT_TASK_STACK_SIZE ];
  inputTaskHandle = xTaskCreateStatic(
                inputReadTask,        // Function that implements the task.
                "Input-Read-Task",    // Text name for the task.
                INPUT_TASK_STACK_SIZE,// Number of indexes in the xStack array.
                NULL,                 // Parameter passed into the task.
                tskIDLE_PRIORITY,     // Priority at which the task is created.
                inputTaskStack,       // Array to use as the task's stack.
                &inputTaskBuffer );   // Variable to hold the task's data structure.
  configASSERT(inputTaskHandle);
  vTaskDelay(pdMS_TO_TICKS(20)); 
    
  // Data Transmit Task
  TaskHandle_t transmitTaskHandle = NULL;
  static StaticTask_t transmitTaskBuffer;
  static StackType_t transmitTaskStack[ TRANSMIT_TASK_STACK_SIZE ];
  transmitTaskHandle = xTaskCreateStatic(
                dataTransmitTask,         /* Function that implements the task. */
                "Data-Transmit-Task",     /* Text name for the task. */
                TRANSMIT_TASK_STACK_SIZE, /* Number of indexes in the xStack array. */
                NULL,                     /* Parameter passed into the task. */
                tskIDLE_PRIORITY,         /* Priority at which the task is created. */
                transmitTaskStack,        /* Array to use as the task's stack. */
                &transmitTaskBuffer );    /* Variable to hold the task's data structure. */
  configASSERT(transmitTaskHandle);
  vTaskDelay(pdMS_TO_TICKS(20)); 

  // Data Receive Task
  TaskHandle_t dataReceiveTaskHandle = NULL;
  static StaticTask_t dataReceiveTaskBuffer;
  static StackType_t dataReceiveTaskStack[ RECEIVE_TASK_STACK_SIZE ];
  dataReceiveTaskHandle = xTaskCreateStatic(
                dataReceiveTask,          /* Function that implements the task. */
                "Input-Read-Task",        /* Text name for the task. */
                RECEIVE_TASK_STACK_SIZE,  /* Number of indexes in the xStack array. */
                NULL,                     /* Parameter passed into the task. */
                tskIDLE_PRIORITY,         /* Priority at which the task is created. */
                dataReceiveTaskStack,     /* Array to use as the task's stack. */
                &dataReceiveTaskBuffer ); /* Variable to hold the task's data structure. */
  configASSERT(dataReceiveTaskHandle);
  vTaskDelay(pdMS_TO_TICKS(20)); 

  // OLED Update Task
  TaskHandle_t oledTaskHandle = NULL;
  static StaticTask_t oledTaskBuffer;
  static StackType_t oledTaskStack[ OLED_TASK_STACK_SIZE ];
  oledTaskHandle = xTaskCreateStatic(
                oledUpdateTask,       /* Function that implements the task. */
                "OLED-Update-Task",   /* Text name for the task. */
                OLED_TASK_STACK_SIZE, /* Number of indexes in the xStack array. */
                NULL,    /* Parameter passed into the task. */
                tskIDLE_PRIORITY,     /* Priority at which the task is created. */
                oledTaskStack,        /* Array to use as the task's stack. */
                &oledTaskBuffer );    /* Variable to hold the task's data structure. */
  configASSERT(oledTaskHandle);
}