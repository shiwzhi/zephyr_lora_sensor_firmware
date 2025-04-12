#ifndef BME280_H
#define BME280_H

typedef struct BME280Data {
    double pressure_hpa;
    double temp;
} bme280_data_t;

int bme280_get_data(bme280_data_t* bme280_data);

#endif