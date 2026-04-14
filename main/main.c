#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "joysticks.h"
#include "switches.h"
#include "tasks_common.h"

#include "switches.h"
#include "setup.h"
#include "espnow.h"

void app_main(void)
{  
/*---- Initialize nvs memory for persistent wifi credentials ----*/ 
  nvs_init();
/*---- Initialize WiFi ----*/ 
  wifi_init();
/*---- Initialize ESPNOW ----*/ 
  init_espnow();
/*---- Initialize I2C ----*/
  i2c_init();
/*---- Initialize OLED ----*/
  oled_init();
/*---- Configure GPIOs ----*/
  switch_gpio_config_t switch_gpio_config = {
    .PIN_SPDT_L = PIN_SPDT_L_CONF,
    .PIN_SPDT_R = PIN_SPDT_R_CONF,
    .PIN_SP3T_LH = PIN_SP3T_LH_CONF,
    .PIN_SP3T_LL = PIN_SP3T_LL_CONF,
    .PIN_SP3T_RH = PIN_SP3T_RH_CONF,
    .PIN_SP3T_RL = PIN_SP3T_RL_CONF,
    .PIN_ARM_DISARM = PIN_ARM_DISARM_CONF
  };
  configure_gpio_inputs(&switch_gpio_config);
/*---- Initialize ADS1115 ----*/
  init_ads1115();

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

  uint8_t peer_addr[6] = ESPNOW_PEER_ADDR_CONF;
  
/*---- Start Tasks ----*/
  // Input Read Task
  static input_task_params_t input_task_params = {
    .control_packet = &control_packet,
  };
  input_task_params.lock = controlPacketMutexHandle;
  TaskHandle_t inputTaskHandle = NULL;
  static StaticTask_t inputTaskBuffer;
  static StackType_t inputTaskStack[ INPUT_TASK_STACK_SIZE ];
  inputTaskHandle = xTaskCreateStatic(
                inputReadTask,        // Function that implements the task.
                "Input-Read-Task",    // Text name for the task.
                INPUT_TASK_STACK_SIZE,// Number of indexes in the xStack array.
                &input_task_params,   // Parameter passed into the task.
                tskIDLE_PRIORITY,     // Priority at which the task is created.
                inputTaskStack,       // Array to use as the task's stack.
                &inputTaskBuffer );   // Variable to hold the task's data structure.
  configASSERT(inputTaskHandle);
    
  // Data Transmit Task
  static transmit_task_params_t transmit_task_params = {
    .control_packet = &control_packet,
  };
  transmit_task_params.lock = controlPacketMutexHandle;
  transmit_task_params.des_addr = peer_addr;
  TaskHandle_t transmitTaskHandle = NULL;
  static StaticTask_t transmitTaskBuffer;
  static StackType_t transmitTaskStack[ TRANSMIT_TASK_STACK_SIZE ];
  transmitTaskHandle = xTaskCreateStatic(
                dataTransmitTask,         /* Function that implements the task. */
                "Data-Transmit-Task",     /* Text name for the task. */
                TRANSMIT_TASK_STACK_SIZE, /* Number of indexes in the xStack array. */
                &transmit_task_params,    /* Parameter passed into the task. */
                tskIDLE_PRIORITY,         /* Priority at which the task is created. */
                transmitTaskStack,        /* Array to use as the task's stack. */
                &transmitTaskBuffer );    /* Variable to hold the task's data structure. */
  configASSERT(transmitTaskHandle);
    
  // Data Receive Task
  static receive_task_params_t receive_task_params = {
    .telemetry_packet = &telemetry_packet,
  };
  receive_task_params.lock = telemetryPacketMutexHandle;
  receive_task_params.des_addr = peer_addr;
  TaskHandle_t dataReceiveTaskHandle = NULL;
  static StaticTask_t dataReceiveTaskBuffer;
  static StackType_t dataReceiveTaskStack[ RECEIVE_TASK_STACK_SIZE ];
  dataReceiveTaskHandle = xTaskCreateStatic(
                dataReceiveTask,          /* Function that implements the task. */
                "Input-Read-Task",        /* Text name for the task. */
                RECEIVE_TASK_STACK_SIZE,  /* Number of indexes in the xStack array. */
                &receive_task_params,     /* Parameter passed into the task. */
                tskIDLE_PRIORITY,         /* Priority at which the task is created. */
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