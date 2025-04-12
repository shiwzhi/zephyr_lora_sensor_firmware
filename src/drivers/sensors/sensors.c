#include "sensors.h"
#include "scd4x.h"
#include "sht3x.h"
#include "pms.h"
#include "bme280.h"
#include "cm1106.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Sensors, LOG_LEVEL_ERR);

static scd4x_data_t scd4xdata;
static sht3x_data_t sht3xdata;
static pms_data_t pmsdata;

int sensor_get_co2(uint16_t *co2)
{
    scd4x_data_t scd4xdata;
    if (get_scd4x_data(&scd4xdata) == 0)
    {
        *co2 = scd4xdata.co2;
        return 0;
    }

    if (cm1106_get_co2(co2) == 0)
    {
        return 0;
    }

    LOG_ERR("Unable to get CO2");
    return -1;
}

int sensor_get_temp(float *temp)
{
    if (sht3x_get_temp_hum(&sht3xdata) == 0)
    {
        *temp = sht3xdata.temp;
        return 0;
    }

    if (get_scd4x_data(&scd4xdata) == 0)
    {
        *temp = scd4xdata.temp;
        return 0;
    }

    LOG_ERR("Unable to get temp");
    return -1;
}
int sensor_get_hum(float *hum)
{
    if (sht3x_get_temp_hum(&sht3xdata) == 0)
    {
        *hum = sht3xdata.hum;
        return 0;
    }

    if (get_scd4x_data(&scd4xdata) == 0)
    {
        *hum = scd4xdata.hum;
        return 0;
    }

    LOG_ERR("Unable to get hum");
    return -1;
}

int sensor_get_pm1(uint16_t *pm1)
{
    if (0 == pms_get(&pmsdata))
    {
        *pm1 = pmsdata.pm1;
        return 0;
    }
    LOG_ERR("Un able to get pms data");
    return -1;
}

int sensor_get_pm25(uint16_t *pm25)
{
    if (0 == pms_get(&pmsdata))
    {
        *pm25 = pmsdata.pm25;
        return 0;
    }
    LOG_ERR("Un able to get pms data");
    return -1;
}

int sensor_get_pm10(uint16_t *pm10)
{
    if (0 == pms_get(&pmsdata))
    {
        *pm10 = pmsdata.pm10;
        return 0;
    }
    LOG_ERR("Un able to get pms data");
    return -1;
}