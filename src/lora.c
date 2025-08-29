#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include "lora.h"
#include "autoconf.h"
#include <stdint.h>
#include "led.h"

LOG_MODULE_REGISTER(LORA, LOG_LEVEL_DBG);

const struct device *lora_dev;
struct lorawan_join_config join_cfg;
uint8_t dev_eui[8];
uint8_t join_eui[8];
uint8_t app_key[16];
static int ret;

int lora_join()
{
    if (hwinfo_get_device_eui64(dev_eui) == 0)
    {
        memcpy(app_key, dev_eui, 8);
    }
    else
    {
        hwinfo_get_device_id(app_key, 16);
        memcpy(dev_eui, app_key, 8);
    }

    LOG_HEXDUMP_DBG(dev_eui, 8, "DEVEUI");
    LOG_HEXDUMP_DBG(app_key, 16, "APPKEY");

    join_cfg.mode = LORAWAN_ACT_OTAA;
    join_cfg.dev_eui = dev_eui;
    join_cfg.otaa.join_eui = join_eui;
    join_cfg.otaa.app_key = app_key;
    join_cfg.otaa.nwk_key = app_key;
    join_cfg.otaa.dev_nonce = 0u;

    lorawan_set_datarate(LORAWAN_DR_3);

    LOG_INF("Joining network over OTAA");
    ret = lorawan_join(&join_cfg);
    if (ret == 0)
    {
        lorawan_enable_adr(true);
    }
    return ret;
}

static void dl_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t len,
                        const uint8_t *hex_data)
{
    LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm, Time %d", port,
            flags & LORAWAN_DATA_PENDING, rssi, snr, !!(flags & LORAWAN_TIME_UPDATED));
    if (hex_data)
    {
        LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
    }
}
struct lorawan_downlink_cb downlink_cb = {
    .port = LW_RECV_PORT_ANY,
    .cb = dl_callback};

int lora_init()
{
    lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
    if (!device_is_ready(lora_dev))
    {
        LOG_ERR("%s: device not ready.", lora_dev->name);
        return -1;
    }
    ret = lorawan_start();
    if (ret < 0)
    {
        LOG_ERR("lorawan_start failed: %d", ret);
        return -1;
    }

    lorawan_register_downlink_callback(&downlink_cb);
    return ret;
}

int lora_send(uint8_t port, uint8_t *buffer, uint8_t len)
{
    ret = lorawan_send(port, buffer, len, LORAWAN_MSG_UNCONFIRMED);
    return ret;
}