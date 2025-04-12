#ifndef SHT3X_H
#define SHT3X_H

typedef struct SHT3XData {
    float temp;
    float hum;
} sht3x_data_t;

int sht3x_get_temp_hum(sht3x_data_t *data);

#endif