#ifndef SCD4X_H
#define SCD4X_H

#include <stdint.h>

typedef struct SCD4XData {
    uint16_t co2;
    float temp;
    float hum;
} scd4x_data_t;

int get_scd4x_data(scd4x_data_t* data);
int set_scd4x_offset(float actual_temp);

#endif