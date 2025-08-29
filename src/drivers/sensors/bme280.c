#include "bme280.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bme280, LOG_LEVEL_ERR);

int bme280_get_data(bme280_data_t *bme280_data)
{
#if DT_HAS_ALIAS(bme280)
    const struct device *const bme280_dev = DEVICE_DT_GET(DT_ALIAS(bme280));
    if (!device_is_ready(bme280_dev))
    {
        LOG_ERR("Device not ready");
        return -1;
    }

    struct sensor_value raw_pressure, raw_temp;
    if (!sensor_sample_fetch(bme280_dev) == 0)
    {
        LOG_ERR("Can't fetch sensor data");
        return -1;
    }

    if (0 != sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS, &raw_pressure))
    {
        LOG_ERR("Can't get pressure data");
        return -1;
    }

    if (0 != sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP, &raw_temp))
    {
        LOG_ERR("Can't get temp data");
        return -1;
    }

    bme280_data->pressure_hpa = sensor_value_to_double(&raw_pressure) * 10;
    bme280_data->temp = sensor_value_to_double(&raw_temp);
    return 0;
#else
    LOG_ERR("No BME280 alias");
    return -1;
#endif
}