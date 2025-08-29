#ifndef SPS30_H
#define SPS30_H

typedef struct SPS30Data_s
{
    float pm1;
    float pm25;
    float pm4;
    float pm10;
} sps30data_t;

int sps30_get_data(sps30data_t *data);

#endif