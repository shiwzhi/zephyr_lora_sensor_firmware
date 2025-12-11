#include "sps30.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(sps30, LOG_LEVEL_ERR);
static bool g_is_dataready = false;
static sps30data_t g_sps30data;

#if DT_HAS_ALIAS(sps30)
static const struct i2c_dt_spec sps30_i2c = I2C_DT_SPEC_GET(DT_ALIAS(sps30));

static uint8_t CalcCrc(uint8_t data[2])
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31u;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

static int sps30_set_pointer(uint16_t pointer)
{
    uint8_t buf[2] = {(uint8_t)((pointer >> 8) & 0xFF), (uint8_t)(pointer & 0xFF)};
    int ret = i2c_write_dt(&sps30_i2c, buf, sizeof(buf));
    if (ret != 0)
    {
        LOG_ERR("Failed to write to I2C data");
    }
    LOG_HEXDUMP_DBG(buf, 2, "Set Pointer: ");
    return ret;
}

static int sps30_read_data(uint16_t pointer, uint8_t *buffer, uint8_t len)
{
    // uint8_t buf[2] = {(uint8_t)((pointer >> 8) & 0xFF), (uint8_t)(pointer & 0xFF)};

    sps30_set_pointer(pointer);

    int ret = i2c_read_dt(&sps30_i2c, buffer, len);

    if (ret != 0)
    {
        LOG_ERR("Failed to read I2C data");
    }
    LOG_HEXDUMP_DBG(buffer, len, "Read data: ");
    return ret;
}

static int sps30_write_data(uint16_t pointer, uint8_t *buffer, uint8_t len)
{
    uint8_t buf[30] = {(uint8_t)((pointer >> 8) & 0xFF), (uint8_t)(pointer & 0xFF)};
    memcpy(buf + 2, buffer, len);

    int ret = i2c_write_dt(&sps30_i2c, buf, len + 2);
    if (ret != 0)
    {
        LOG_ERR("Failed to write to I2C data");
    }
    LOG_HEXDUMP_DBG(buf, len + 2, "Write data: ");

    return ret;
}

void sps30_start_measurement()
{
    uint16_t pointer = 0x0010;
    uint8_t data[3] = {0x03, 0x00, 0x00};
    data[2] = CalcCrc(data);
    sps30_write_data(pointer, data, sizeof(data));
}

int sps30_data_ready()
{
    uint16_t pointer = 0x0202;
    uint8_t buffer[3];

    if (0 == sps30_read_data(pointer, buffer, sizeof(buffer)))
    {
        if (CalcCrc(buffer) == buffer[2])
        {
            LOG_DBG("Data Ready Flag: %i", buffer[1]);
            return buffer[1] == 1 ? 0 : -1;
        }
    }
    return -1;
}

void sps30_stop_measurement()
{
    uint16_t pointer = 0x0104;
    sps30_set_pointer(pointer);
}

void sps30_read_measured_values()
{
    uint16_t pointer = 0x0300;
    uint8_t buffer[60];
    if (0 == sps30_read_data(pointer, buffer, sizeof(buffer)))
    {
        float pm1, pm25, pm4, pm10;
        uint32_t buf;

        buf = (uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[3] << 8 | (uint32_t)buffer[4];
        memcpy(&pm1, &buf, 4);

        buf = (uint32_t)buffer[6] << 24 | (uint32_t)buffer[7] << 16 | (uint32_t)buffer[9] << 8 | (uint32_t)buffer[10];
        memcpy(&pm25, &buf, 4);

        buf = (uint32_t)buffer[12] << 24 | (uint32_t)buffer[13] << 16 | (uint32_t)buffer[15] << 8 | (uint32_t)buffer[16];
        memcpy(&pm4, &buf, 4);

        buf = (uint32_t)buffer[18] << 24 | (uint32_t)buffer[19] << 16 | (uint32_t)buffer[21] << 8 | (uint32_t)buffer[22];
        memcpy(&pm10, &buf, 4);

        LOG_DBG("PM: %.2f %.2f %.2f %.2f", (double)pm1, (double)pm25, (double)pm4, (double)pm10);

        g_sps30data.pm1 = pm1;
        g_sps30data.pm25 = pm25;
        g_sps30data.pm4 = pm4;
        g_sps30data.pm10 = pm10;
    }
}

void sps30_start_fan_cleaning()
{
    uint16_t pointer = 0x5607;
    sps30_set_pointer(pointer);
}

void sps30_task(void *, void *, void *)
{
    while (1)
    {
        sps30_start_measurement();
        k_msleep(30 * 1000);
        if (sps30_data_ready() == 0)
        {
            sps30_read_measured_values();
            g_is_dataready = true;
        }
        sps30_stop_measurement();
        k_msleep(30 * 1000);
    }
}

K_THREAD_DEFINE(sps30_tid, 1024,
                sps30_task, NULL, NULL, NULL,
                7, 0, 0);

#endif
int sps30_get_data(sps30data_t *data)
{
    if (g_is_dataready)
    {
        memcpy(data, &g_sps30data, sizeof(g_sps30data));
        return 0;
    }
    LOG_WRN("SPS30 data not ready");
    return -1;
}