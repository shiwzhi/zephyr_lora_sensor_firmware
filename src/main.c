/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <cJSON.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/hwinfo.h>

#define PMS_DEVICE_LABEL "pms_device"
#define SHT3X_LABEL "sht3x_sensor"
#define SSD1306_DEVICE_LABEL "ssd1306_display"
#define CM1106_DEVICE_LABEL "cm1106_co2"
#define LORA_DEVICE_LABEL "lora_node"

#define STACKSIZE 1024
#define PRIORITY 7

uint16_t g_pm1, g_pm25, g_pm10, g_co2;
float g_temp, g_hum;

bool g_is_co2 = false;
bool g_is_temp = false;
bool g_is_hum = false;
bool g_is_lora = false;
bool g_is_voc = false;
bool g_is_pressure = false;

uint8_t cm1106_rx_index = 0;
uint8_t cm1106_rx_buffer[64];
void cm1106_serial_cb(const struct device *dev, void *user_data)
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
		cm1106_rx_buffer[cm1106_rx_index] = c;
		cm1106_rx_index++;
	}
}

void cm1106_thread(void *, void *, void *)
{
	const struct device *cm1106_uart_dev = device_get_binding(CM1106_DEVICE_LABEL);
	if (cm1106_uart_dev == NULL)
	{
		printf("CM1106 not found\r\n");
		return;
	}

	if (!device_is_ready(cm1106_uart_dev))
	{
		printk("CM1106 UART device not found!");
		return;
	}

	int ret = uart_irq_callback_user_data_set(cm1106_uart_dev, cm1106_serial_cb, NULL);
	if (ret < 0)
	{
		if (ret == -ENOTSUP)
		{
			printk("Interrupt-driven UART API support not enabled\n");
		}
		else if (ret == -ENOSYS)
		{
			printk("UART device does not support interrupt-driven API\n");
		}
		else
		{
			printk("Error setting UART callback: %d\n", ret);
		}
		return;
	}
	uart_irq_rx_enable(cm1106_uart_dev);

	uint8_t read_co2_cmd[4] = {0x11, 0x01, 0x01, 0xed};
	while (1)
	{
		cm1106_rx_index = 0;
		for (int i = 0; i < 4; i++)
		{
			uart_poll_out(cm1106_uart_dev, read_co2_cmd[i]);
		}
		k_msleep(5000);
		if (cm1106_rx_index == 8)
		{
			g_co2 = cm1106_rx_buffer[3] * 256 + cm1106_rx_buffer[4];
			printf("CO2: %d\r\n", g_co2);
		}
	}
}

K_THREAD_DEFINE(cm1106_tid, STACKSIZE,
				cm1106_thread, NULL, NULL, NULL,
				PRIORITY, 0, 5000);

void sht3x_thread(void *, void *, void *)
{
	const struct device *sht_dev = device_get_binding(SHT3X_LABEL);
	if (sht_dev == NULL)
	{
		printf("sht3x not found \r\n");
		return;
	}

	if (!device_is_ready(sht_dev))
	{
		printf("Device %s is not ready\n", sht_dev->name);
		return;
	}

	struct sensor_value temp, hum;

	while (1)
	{
		int rc = sensor_sample_fetch(sht_dev);
		if (rc == 0)
		{
			rc = sensor_channel_get(sht_dev, SENSOR_CHAN_AMBIENT_TEMP,
									&temp);
		}
		if (rc == 0)
		{
			rc = sensor_channel_get(sht_dev, SENSOR_CHAN_HUMIDITY,
									&hum);
		}
		if (rc != 0)
		{
			printf("SHT3XD: failed: %d\n", rc);
			continue;
		}
		g_temp = sensor_value_to_double(&temp);
		g_hum = sensor_value_to_double(&hum);
		printf("SHT3XD: %.2f Cel ; %0.2f %%RH\n",
			   g_temp,
			   g_hum);
		k_msleep(5000);
	}
}

K_THREAD_DEFINE(sht3x_tid, STACKSIZE,
				sht3x_thread, NULL, NULL, NULL,
				PRIORITY, 0, 5000);

void pms_thread(void *, void *, void *)
{
	const struct device *pms_dev = device_get_binding(PMS_DEVICE_LABEL);
	if (pms_dev == NULL)
	{
		printf("pms not found\r\n");
		return;
	}

	if (!device_is_ready(pms_dev))
	{
		printf("Device %s is not ready\n", pms_dev->name);
		return;
	}

	while (1)
	{
		int ret = sensor_sample_fetch(pms_dev);
		if (ret == 0)
		{
			struct sensor_value pm1, pm25, pm10;
			ret = sensor_channel_get(pms_dev, SENSOR_CHAN_PM_1_0, &pm1);
			if (ret == 0)
				ret = sensor_channel_get(pms_dev, SENSOR_CHAN_PM_2_5, &pm25);
			if (ret == 0)
				ret = sensor_channel_get(pms_dev, SENSOR_CHAN_PM_10, &pm10);
			if (ret != 0)
			{
				printf("pms: failed: %d\n", ret);
			}
			else
			{
				g_pm1 = sensor_value_to_double(&pm1);
				g_pm25 = sensor_value_to_double(&pm25);
				g_pm10 = sensor_value_to_double(&pm10);
				printf("pm1: %d pm25: %d pm10: %d\r\n", g_pm1, g_pm25, g_pm10);
			}
		}
		else
		{
			printf("pms sample fetch return %d\r\n", ret);
		}
		k_msleep(5000);
	}
}

K_THREAD_DEFINE(pms_tid, STACKSIZE,
				pms_thread, NULL, NULL, NULL,
				PRIORITY, 0, 5000);

void ssd1306_display_thread(void *, void *, void *)
{
	const struct device *dev;

	dev = device_get_binding(SSD1306_DEVICE_LABEL);
	if (!device_is_ready(dev))
	{
		printf("Device %s not ready\n", dev->name);
		return;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0)
	{
		if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO01) != 0)
		{
			printf("Failed to set required pixel format");
			return;
		}
	}

	printf("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev))
	{
		printf("Framebuffer initialization failed!\n");
		return 0;
	}

	display_blanking_off(dev);

	cfb_framebuffer_set_font(dev, 0);
	cfb_framebuffer_clear(dev, true);

	while (1)
	{
		cfb_framebuffer_clear(dev, false);

		char temp_hum_str[30];
		sprintf(temp_hum_str, "%.1fC %.1f%%", g_temp, g_hum);
		cfb_print(dev, temp_hum_str, 0, 0);

		char co2_str[20];
		sprintf(co2_str, "CO2: %d", g_co2);
		cfb_print(dev, co2_str, 0, 16);

		char pm_str[30];
		sprintf(pm_str, "%d %d %d", g_pm1, g_pm25, g_pm10);
		cfb_print(dev, pm_str, 0, 32);

		cfb_framebuffer_finalize(dev);
		k_msleep(5000);
	}
}

K_THREAD_DEFINE(ssd1306_tid, STACKSIZE,
				ssd1306_display_thread, NULL, NULL, NULL,
				PRIORITY, 0, 5000);

double roundf_val(float input, int decimal)
{
	int _pow = 1;
	for (int i = 0; i < decimal; i++)
	{
		_pow = _pow * 10;
	}

	double value;
	if (input >= 0)
	{
		value = (int)(input * _pow + 0.5f);
	}
	else
	{
		value = (int)(input * _pow - 0.5f);
	}

	return value / _pow;
}
void lora_node_task(void *, void *, void *)
{
	const struct device *const lora_dev = device_get_binding(LORA_DEVICE_LABEL);
	struct lora_modem_config config;
	int ret;

	if (!device_is_ready(lora_dev))
	{
		printf("%s Device not ready", lora_dev->name);
		return;
	}

	config.frequency = 410000000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_7;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 18;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
	if (ret < 0)
	{
		printf("LoRa config failed");
		return;
	}

	while (1)
	{
		k_msleep(60 * 1000);
		
		cJSON *root = cJSON_CreateObject();
		if (root == NULL)
		{
			printk("Failed to create JSON object\n");
			return;
		}

		cJSON_AddNumberToObject(root, "temp", roundf_val(g_temp, 1));
		cJSON_AddNumberToObject(root, "hum", roundf_val(g_hum, 1));
		cJSON_AddNumberToObject(root, "co2", g_co2);
		cJSON_AddNumberToObject(root, "pm1", g_pm1);
		cJSON_AddNumberToObject(root, "pm25", g_pm25);
		cJSON_AddNumberToObject(root, "pm10", g_pm10);

		uint8_t deviceEUI[8] = {};
		int ret = hwinfo_get_device_id(deviceEUI, 8);
		if (ret > 0)
		{
			char eui_str[20];
			for (int i = 0; i < ret; i++)
			{
				sprintf(eui_str + i * 2, "%02X", deviceEUI[i]);
			}
			eui_str[16] = '\0';
			cJSON_AddStringToObject(root, "id", eui_str);
		}
		else
		{
			printf("can not get device id, ret: %d\r\n", ret);
		}

		char *json_str = cJSON_Print(root);
		if (json_str)
		{
			printk("Serialized JSON: %s\n", json_str);
			ret = lora_send(lora_dev, json_str, strlen(json_str));
			if (ret < 0)
			{
				printf("LoRa send failed");
				return 0;
			}

			printf("Data sent!");

			cJSON_free(json_str); // Free allocated memory
		}

		cJSON_Delete(root); // Free JSON object
	}
}

K_THREAD_DEFINE(lora_node_tid, STACKSIZE,
				lora_node_task, NULL, NULL, NULL,
				PRIORITY, 0, 5000);

int main(void)
{

	while (1)
	{
		// printk("Hello World! %s\n", CONFIG_BOARD_TARGET);
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
