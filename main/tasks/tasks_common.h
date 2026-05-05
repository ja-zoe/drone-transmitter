#pragma once
#include "drone_common.h"

/*----- Task function definitions -----*/
void inputReadTask( void *pvParameters);
void dataTransmitTask( void *pvParameters );
void dataReceiveTask( void *pvParameters );
void oledUpdateTask( void *pvParameters );
void emergencyFailSafe();