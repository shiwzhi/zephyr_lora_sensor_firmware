#include "sht3x.h"
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(sht3x, LOG_LEVEL_ERR);

static const struct device *sht_dev;
static struct sensor_value g_temp, g_hum;

int sht3x_get_temp_hum(sht3x_data_t *data)
{
    k_msleep(1000);
#if DT_HAS_ALIAS(sht3x_sensor)
    sht_dev = DEVICE_DT_GET(DT_ALIAS(sht3x_sensor));
#else
    LOG_ERR("No sht3x-sensor alias found");
    return -1;
#endif
    if (!device_is_ready(sht_dev))
    {
        LOG_ERR("Device not ready");
        return -1;
    }

    int rc = sensor_sample_fetch(sht_dev);
    if (rc == 0)
    {
        rc = sensor_channel_get(sht_dev, SENSOR_CHAN_AMBIENT_TEMP,
                                &g_temp);
    }
    if (rc == 0)
    {
        rc = sensor_channel_get(sht_dev, SENSOR_CHAN_HUMIDITY,
                                &g_hum);
    }
    if (rc != 0)
    {
        LOG_ERR("SHT3XD: failed: %d\n", rc);
        return -1;
    }
    data->temp = sensor_value_to_double(&g_temp);
    data->hum = sensor_value_to_double(&g_hum);
    return 0;
}
