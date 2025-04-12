#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/hwinfo.h>

#include "cayenne_lpp.h"
#include "sensors.h"
#include "lora.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static uint8_t deveui[8];
static uint8_t joineui[8];
static uint8_t appkey[16];

int main(void)
{
	k_msleep(5000);

	memset(deveui, 0, 8);
	memset(joineui, 0, 8);
	memset(appkey, 0, 8);
	hwinfo_get_device_id(deveui, 8);
	hwinfo_get_device_id(appkey, 16);
	LOG_HEXDUMP_DBG(deveui, 8, "DEVEUI:");
	LOG_HEXDUMP_DBG(joineui, 8, "JOINEUI:");
	LOG_HEXDUMP_DBG(appkey, 16, "APPKEY:");
	lora_init(deveui, joineui, appkey);

	cayenne_lpp_t lpp = {0};

	while (1)
	{
		k_msleep(60 * 1000);

		cayenne_lpp_reset(&lpp);
		uint16_t _pm1, _pm25, _pm10;
		if (0 == sensor_get_pm1(&_pm1))
		{
			sensor_get_pm25(&_pm25);
			sensor_get_pm10(&_pm10);

			float pm1 = _pm1 / 100.0f;
			float pm25 = _pm25 / 100.0f;
			float pm10 = _pm10 / 100.0f;

			cayenne_lpp_add_analog_output(&lpp, 0, pm1);
			cayenne_lpp_add_analog_output(&lpp, 1, pm25);
			cayenne_lpp_add_analog_output(&lpp, 2, pm10);
		}

		uint16_t co2;
		if (sensor_get_co2(&co2) == 0)
		{
			cayenne_lpp_add_analog_output(&lpp, 3, co2 / 100.0f);
		}

		float temp;
		if (sensor_get_temp(&temp) == 0)
		{
			cayenne_lpp_add_temperature(&lpp, 0, temp);
		}

		float hum;
		if (sensor_get_hum(&hum) == 0)
		{
			cayenne_lpp_add_relative_humidity(&lpp, 0, hum);
		}

		lora_send(lpp.buffer, lpp.cursor);
	}

	return 0;
}