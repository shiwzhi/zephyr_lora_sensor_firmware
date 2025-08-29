#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/scd4x.h>
#include "scd4x.h"
#include "bme280.h"

LOG_MODULE_REGISTER(SCD4x, LOG_LEVEL_INF);

static bool is_scd4x_init = false;
static uint16_t co2;
static double temp, hum;

#if DT_HAS_ALIAS(scd4x)

void scd4x_thread(void *, void *, void *)
{
    const struct device *const scd4x_dev = DEVICE_DT_GET(DT_ALIAS(scd4x));
    static struct sensor_value co2_raw, temp_raw, hum_raw;

    if (!device_is_ready(scd4x_dev))
    {
        LOG_ERR("device not ready");
        return;
    }

    // if (scd4x_factory_reset(scd4x_dev) == 0 ) {
    //     LOG_INF("SCD4x factory reset success");
    // }

    is_scd4x_init = true;
    bme280_data_t bme280_data;

    struct sensor_value scd4x_tempoffset;
    sensor_value_from_float(&scd4x_tempoffset, 5.0);

    sensor_attr_set(scd4x_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SCD4X_TEMPERATURE_OFFSET, &scd4x_tempoffset);
    sensor_attr_get(scd4x_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SCD4X_TEMPERATURE_OFFSET, &scd4x_tempoffset);
    LOG_INF("Offset: %d", (int)(sensor_value_to_float(&scd4x_tempoffset) * 10));

    while (1)
    {
        if (bme280_get_data(&bme280_data) == 0)
        {
            struct sensor_value scd4x_pressure;
            sensor_value_from_double(&scd4x_pressure, bme280_data.pressure_hpa);
            if (0 == sensor_attr_set(scd4x_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE, &scd4x_pressure))
            {
                LOG_INF("PRESSURE: %d TEMP: %d", scd4x_pressure.val1, (int)(bme280_data.temp * 10));
            }
        }

        if (sensor_sample_fetch(scd4x_dev) == 0)
        {
            sensor_channel_get(scd4x_dev, SENSOR_CHAN_CO2, &co2_raw);
            sensor_channel_get(scd4x_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_raw);
            sensor_channel_get(scd4x_dev, SENSOR_CHAN_HUMIDITY, &hum_raw);
            co2 = sensor_value_to_double(&co2_raw);
            temp = sensor_value_to_double(&temp_raw);
            hum = sensor_value_to_double(&hum_raw);
            LOG_INF("CO2: %d TEMP: %d HUM: %d", co2, (int)(temp * 10), (int)(hum * 10));
        }

        k_sleep(K_MSEC(30 * 1000));
    }
}

K_THREAD_DEFINE(scd4x_tid, 2048,
                scd4x_thread, NULL, NULL, NULL,
                7, 0, 0);

#endif

int get_scd4x_data(scd4x_data_t *data)
{
    if (!is_scd4x_init)
    {
        // LOG_ERR("SCD4x not init");
        return -1;
    }

    data->co2 = co2;
    data->temp = temp;
    data->hum = hum;
    return 0;
}