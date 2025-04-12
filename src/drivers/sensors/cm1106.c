#include "cm1106.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(CM1106, LOG_LEVEL_ERR);

static bool g_is_cm1106_init = false;
static uint16_t g_co2 = 0;

#if DT_HAS_ALIAS(cm1106_uart)

#define CM1106_UART DT_ALIAS(cm1106_uart)

static uint8_t cm1106_rx_len = 0;
static uint8_t cm1106_rx_buf[50];
static const struct device *cm1106_uart_dev;

static void cm1106_serial_cb(const struct device *dev, void *user_data)
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
        cm1106_rx_buf[cm1106_rx_len] = c;
        cm1106_rx_len++;
    }
}

static void cm1106_send_data(uint8_t *buffer, uint8_t len)
{
    uint16_t checksum = 0;
    for (int i = 0; i < len - 1; i++)
    {
        checksum += buffer[i];
    }
    checksum = checksum % 256;
    checksum = 256 - checksum;
    buffer[len - 1] = checksum;
    LOG_HEXDUMP_DBG(buffer, len, "CM1106 TX: ");
    cm1106_rx_len = 0;
    for (int i = 0; i < len; i++)
    {
        uart_poll_out(cm1106_uart_dev, buffer[i]);
    }
    k_msleep(2000);
    LOG_HEXDUMP_DBG(cm1106_rx_buf, cm1106_rx_len, "CM1106 RX: ");
}

int cm1106_cal_co2(uint16_t co2)
{
    if (!g_is_cm1106_init)
    {
        LOG_ERR("CM1106 not init");
        return -1;
    }
    LOG_DBG("CM1106 Cal CO2: %d", co2);
    uint8_t high_byte = (uint8_t)(co2 >> 8);
    uint8_t low_byte = (uint8_t)(co2 & 0xFF);
    uint8_t cal_co2_cmd[6] = {0x11, 0x03, 0x03, high_byte, low_byte, 0x00};

    cm1106_send_data(cal_co2_cmd, 6);
    // todo reutrn state
    return 0;
}

static int cm1106_set_abc(uint8_t day)
{
    if (!g_is_cm1106_init)
    {
        LOG_ERR("CM1106 not init");
        return -1;
    }
    LOG_DBG("CM1106 enable ABC to %d days", day);
    uint8_t enable_abc_cmd[10] = {0x11, 0x07, 0x10, 0x64, 0x00, day, 0x01, 0x90, 0x64, 0x78};

    cm1106_send_data(enable_abc_cmd, 10);

    if (cm1106_rx_len == 4) {
        if (cm1106_rx_buf[0] == 0x16 && cm1106_rx_buf[1] == 0x01 && cm1106_rx_buf[2] == 0x10 && cm1106_rx_buf[3] == 0xD9) {
            return 0;
        }
    }
    return -1;
}

static int cm1106_init()
{
    int ret;
    cm1106_uart_dev = DEVICE_DT_GET(CM1106_UART);

    if (cm1106_uart_dev == NULL)
    {
        LOG_ERR("CM1106 not found\r\n");
        return -1;
    }

    if (!device_is_ready(cm1106_uart_dev))
    {
        LOG_ERR("CM1106 UART device not found!");
        return -1;
    }

    ret = uart_irq_callback_user_data_set(cm1106_uart_dev, cm1106_serial_cb, NULL);
    if (ret < 0)
    {
        if (ret == -ENOTSUP)
        {
            LOG_ERR("Interrupt-driven UART API support not enabled\n");
        }
        else if (ret == -ENOSYS)
        {
            LOG_ERR("UART device does not support interrupt-driven API\n");
        }
        else
        {
            LOG_ERR("Error setting UART callback: %d\n", ret);
        }
        return -1;
    }
    uart_irq_rx_enable(cm1106_uart_dev);

    g_is_cm1106_init = true;

    uint8_t abc_day = 30;
    if (0 != cm1106_set_abc(abc_day)) {
        LOG_ERR("ABC set failed");
    } else {
        LOG_INF("ABC set to %d", abc_day);
    }
    // cm1106_cal_co2(560);

    return 0;
}

void cm1106_thread(void *, void *, void *)
{
    if (!g_is_cm1106_init)
    {
        if (cm1106_init() != 0)
        {
            LOG_ERR("CM1106 init failed");
            return;
        }
    }

    while (1)
    {
        LOG_DBG("CM1106 Read CO2");
        uint8_t read_co2_cmd[4] = {0x11, 0x01, 0x01, 0xed};

        cm1106_send_data(read_co2_cmd, 4);

        if (cm1106_rx_len == 8)
        {
            g_co2 = cm1106_rx_buf[3] * 256 + cm1106_rx_buf[4];
            LOG_INF("CO2: %d", g_co2);
        }
        else
        {
            LOG_ERR("get CO2 failed");
        }

        k_msleep(5000);
    }
}

K_THREAD_DEFINE(cm1106_tid, 1024,
                cm1106_thread, NULL, NULL, NULL,
                7, 0, 0);

#endif

int cm1106_get_co2(uint16_t *co2)
{
    if (!g_is_cm1106_init)
    {
        LOG_ERR("CM1106 not init");
        return -1;
    }
    else
    {
        *co2 = g_co2;
        return 0;
    }
}