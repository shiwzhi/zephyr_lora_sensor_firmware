#ifndef PMS_H
#define PMS_H

#include "stdint.h"

typedef struct PMSData
{
    uint16_t pm1;
    uint16_t pm25;
    uint16_t pm10;
} pms_data_t;

int pms_get(pms_data_t *pms_data);

#endif