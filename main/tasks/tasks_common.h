#pragma once
#include "joysticks.h"
#include "switches.h"

typedef struct {
  joysticks_values_t joysticks_values;
  switch_states_t switches_values;
} control_packet_t;

typedef struct {

} telemetry_packet_t;

/*----- Task pvParameters -----*/
typedef struct {
    control_packet_t *control_packet;
    SemaphoreHandle_t lock;
} input_task_params_t;

typedef struct {
  telemetry_packet_t *telemetry_packet;
  SemaphoreHandle_t lock;
} oled_task_params_t;
typedef struct {
    control_packet_t *control_packet;
    SemaphoreHandle_t lock;
    uint8_t *des_addr;
} transmit_task_params_t;

typedef struct {
  telemetry_packet_t *telemetry_packet;
  SemaphoreHandle_t lock;
  uint8_t *des_addr;
} receive_task_params_t;


void inputReadTask( void *pvParameters);
void dataTransmitTask( void *pvParameters );
void dataReceiveTask( void *pvParameters );
void oledUpdateTask( void *pvParameters );
void emergencyFailSafe();