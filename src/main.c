#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensors.h"
#include "cayenne_lpp.h"
#include "lora.h"
#include "led.h"

cayenne_lpp_t lpp = {0};
static uint16_t _pm1, _pm25, _pm10;
static float pm1, pm25, pm10, temp, hum;
static uint16_t co2;
static int ret;

int main(void)
{
	led_toggle_system(500);
	lora_init();
	led_toggle_system(3000);
	while (lora_join() != 0)
	{
		k_msleep(5 * 60 * 1000);
	}

	while (1)
	{
		k_msleep(60 * 1000);

		cayenne_lpp_reset(&lpp);
		if (0 == sensor_get_pm1(&_pm1))
		{
			sensor_get_pm25(&_pm25);
			sensor_get_pm10(&_pm10);

			pm1 = _pm1 / 100.0f;
			pm25 = _pm25 / 100.0f;
			pm10 = _pm10 / 100.0f;

			cayenne_lpp_add_analog_output(&lpp, 0, pm1);
			cayenne_lpp_add_analog_output(&lpp, 1, pm25);
			cayenne_lpp_add_analog_output(&lpp, 2, pm10);
		}

		if (sensor_get_co2(&co2) == 0)
		{
			cayenne_lpp_add_analog_output(&lpp, 3, co2 / 100.0f);
		}

		if (sensor_get_temp(&temp) == 0)
		{
			cayenne_lpp_add_temperature(&lpp, 0, temp);
		}

		if (sensor_get_hum(&hum) == 0)
		{
			cayenne_lpp_add_relative_humidity(&lpp, 0, hum);
		}

		ret = lora_send(1, lpp.buffer, lpp.cursor);
		if (ret != 0)
		{
			lora_join();
			k_msleep(5 * 60 * 1000);
		}
	}

	return 0;
}