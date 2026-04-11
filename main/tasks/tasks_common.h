#pragma once

#include "joysticks.h"
#include "switches.h"

typedef struct {
  joysticks_values_t joysticks_values;
  switch_states_t switches_values;
} control_packet_t;

typedef struct {
    control_packet_t data;
    SemaphoreHandle_t lock;
} control_packet_mutex_t;

typedef struct {

} telemetry_packet_t;

typedef struct {
  telemetry_packet_t data;
  SemaphoreHandle_t lock;
} telemetry_packet_mutex_t;

void inputReadTask( void *pvParameters);
void dataTransmitTask( void *pvParameters );
void dataReceiveTask( void *pvParameters );
void oledUpdateTask( void *pvParameters );