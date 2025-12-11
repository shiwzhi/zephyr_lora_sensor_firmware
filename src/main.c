#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/hwinfo.h>
#include "sps30.h"
#include "scd4x.h"
#include "pms.h"
#include "sht3x.h"
#include "scd4x.h"
#include "lora.h"
#include "cayenne_lpp.h"

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

LoRaConfig_t config;
LoRadata_t lora_data;

cayenne_lpp_t lpp = {0};
static float pm1, pm25, pm10;
sps30data_t sps30_data;
scd4x_data_t scd4x_data;
pms_data_t pms_data;
sht3x_data_t sht3x_data;

int main(void)
{
	lora_init();
	if (hwinfo_get_device_eui64(config.deveui) == 0)
	{
		memset(config.appkey, 0, 16);
		memcpy(config.appkey, config.deveui, 8);
	}
	else
	{
		hwinfo_get_device_id(config.appkey, 16);
		memcpy(config.deveui, config.appkey, 8);
	}
	config.join_dr = 3;
	config.adr = 1;
	LOG_HEXDUMP_DBG(config.deveui, 8, "DEVEUI");
	LOG_HEXDUMP_DBG(config.appkey, 16, "APPKEY");

	while (lora_join_network(&config) != 0)
	{
		k_msleep(6000);
	}

	while (1)
	{
		k_msleep(60 * 1000);

		cayenne_lpp_reset(&lpp);
		if (0 == sps30_get_data(&sps30_data))
		{
			pm1 = sps30_data.pm1 / 100.0f;
			pm25 = sps30_data.pm25 / 100.0f;
			pm10 = sps30_data.pm10 / 100.0f;
			cayenne_lpp_add_analog_output(&lpp, 0, pm1);
			cayenne_lpp_add_analog_output(&lpp, 1, pm25);
			cayenne_lpp_add_analog_output(&lpp, 2, pm10);
		}

		if (0 == pms_get(&pms_data))
		{
			pm1 = pms_data.pm1 / 100.0f;
			pm25 = pms_data.pm25 / 100.0f;
			pm10 = pms_data.pm10 / 100.0f;
			cayenne_lpp_add_analog_output(&lpp, 0, pm1);
			cayenne_lpp_add_analog_output(&lpp, 1, pm25);
			cayenne_lpp_add_analog_output(&lpp, 2, pm10);
		}

		if (get_scd4x_data(&scd4x_data) == 0)
		{
			cayenne_lpp_add_analog_output(&lpp, 3, scd4x_data.co2 / 100.0f);
			cayenne_lpp_add_temperature(&lpp, 0, scd4x_data.temp);
			cayenne_lpp_add_relative_humidity(&lpp, 0, scd4x_data.hum);
		}

		if (sht3x_get_temp_hum(&sht3x_data) == 0)
		{
			cayenne_lpp_add_temperature(&lpp, 0, sht3x_data.temp);
			cayenne_lpp_add_relative_humidity(&lpp, 0, sht3x_data.hum);
		}

		lora_data.data_len = lpp.cursor;
		lora_data.port = 1;
		lora_data.is_confirmed = 0;
		memcpy(lora_data.data, lpp.buffer, lpp.cursor);
		lora_send_data(&lora_data);
		k_msleep(6000);
		if (0 == lora_get_downlink(&lora_data))
		{
			LOG_INF("Process downlink");
			if (lora_data.port == 62 && lora_data.data_len == 4)
			{
				LOG_DBG("Calibrate SCD4x");
				float actual_temp;
				memcpy(&actual_temp, lora_data.data, 4);
				LOG_DBG("Actual Temp: %i", (int)(actual_temp * 10));
				set_scd4x_offset(actual_temp);
			}
		}
	}

	return 0;
}