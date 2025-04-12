#include "pms.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(PMS, LOG_LEVEL_ERR);

static bool pms_is_running = false;
static uint16_t g_pm1, g_pm25, g_pm10;

#if DT_HAS_ALIAS(pms_uart)

#define PMS_SLEEP_MS 30 * 1000
static const struct device *pms_uart_dev;
static uint8_t pms_rx_buf[50];
static uint8_t pms_rx_len;

static void pms_serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(dev))
    {
        return;
    }

    if (!uart_irq_rx_ready(dev))
    {
        return;
    }

    while (uart_fifo_read(dev, &c, 1) == 1)
    {
        if (pms_rx_len < 50)
        {
            pms_rx_buf[pms_rx_len] = c;
            pms_rx_len++;
        }
    }
}

static int pms_init()
{
    int ret;
    pms_uart_dev = DEVICE_DT_GET(DT_ALIAS(pms_uart));

    if (NULL == pms_uart_dev)
    {
        LOG_ERR("PMS device not found\r\n");
        return -1;
    }
    if (!device_is_ready(pms_uart_dev))
    {
        LOG_ERR("PMS UART device not ready!");
        return -1;
    }

    ret = uart_irq_callback_user_data_set(pms_uart_dev, pms_serial_cb, NULL);
    if (0 != ret)
    {
        LOG_ERR("PMS UART callback failed!");
        return -1;
    }

    uart_irq_rx_enable(pms_uart_dev);

    LOG_DBG("PMS init success");
    return 0;
}

void static pms_cal_crc(uint8_t *buffer, uint8_t len)
{
    uint16_t checksum = 0;
    for (int i = 0; i < len - 2; i++)
    {
        checksum += buffer[i];
    }
    uint8_t high_byte = (uint8_t)(checksum >> 8);
    uint8_t low_byte = (uint8_t)(checksum & 0xFF);
    buffer[len - 2] = high_byte;
    buffer[len - 1] = low_byte;
}

static void pms_uart_out(uint8_t *buffer, uint8_t len)
{
    pms_rx_len = 0;
    pms_cal_crc(buffer, len);

    LOG_HEXDUMP_DBG(buffer, len, "PMS TX: ");
    for (int i = 0; i < len; i++)
    {
        uart_poll_out(pms_uart_dev, buffer[i]);
    }
    k_msleep(2000);
    LOG_HEXDUMP_DBG(pms_rx_buf, pms_rx_len, "PMS RX: ");
}

static int pms_config_passive(bool state)
{
    uint8_t pms_passive_cmd[7] = {0x42, 0x4d, 0xe1, 0x00, (state ? 0x00 : 0x01), 0x00, 0x00};
    pms_uart_out(pms_passive_cmd, 7);
    return 0;
}

static int pms_config_sleep(bool state)
{
    uint8_t pms_sleep_cmd[7] = {0x42, 0x4d, 0xe4, 0x00, (state ? 0x00 : 0x01), 0x00, 0x00};
    pms_uart_out(pms_sleep_cmd, 7);
    return 0;
}

int pms_start()
{
    pms_config_sleep(false);
    pms_config_passive(true);
    return 0;
}

int pms_stop()
{
    pms_config_sleep(true);
    return 0;
}

static int pms_read()
{
    uint8_t pms_read_cmd[7] = {0x42, 0x4d, 0xe2, 0x00, 0x00, 0x00, 0x00};
    pms_uart_out(pms_read_cmd, 7);

    if (pms_rx_len >= 32 && pms_rx_buf[0] == 0x42)
    {
        g_pm1 = pms_rx_buf[10] * 0x100 + pms_rx_buf[11];
        g_pm25 = pms_rx_buf[12] * 0x100 + pms_rx_buf[13];
        g_pm10 = pms_rx_buf[14] * 0x100 + pms_rx_buf[15];
        printf("\r\nPM1: %d PM25: %d PM10: %d\r\n", (int)g_pm1, (int)g_pm25, (int)g_pm10);
    }
    return 0;
}

void pms_task(void *, void *, void *)
{
    if (0 != pms_init())
    {
        return;
    }

    pms_is_running = true;

    pms_start();
    k_msleep(10 * 1000);
    pms_read();

    while (1)
    {
        pms_stop();
        k_msleep(PMS_SLEEP_MS);
        pms_start();
        k_msleep(30 * 1000);
        pms_read();
    }
}

K_THREAD_DEFINE(pms_tid, 1024,
                pms_task, NULL, NULL, NULL,
                7, 0, 0);

#endif

int pms_get(pms_data_t *pms_data)
{
    if (!pms_is_running)
    {
        LOG_ERR("PMS not running");
        return -1;
    }

    pms_data->pm1 = g_pm1;
    pms_data->pm25 = g_pm25;
    pms_data->pm10 = g_pm10;
    return 0;
}