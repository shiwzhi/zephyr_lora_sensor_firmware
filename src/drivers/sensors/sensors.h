#ifndef _H_SENSORS
#define _H_SENSORS

#include <stdint.h>

int sensor_get_co2(uint16_t *co2);
int sensor_get_temp(float *temp);
int sensor_get_hum(float *hum);
int sensor_get_pm1(uint16_t *pm1);
int sensor_get_pm25(uint16_t *pm25);
int sensor_get_pm10(uint16_t *pm10);

#endif