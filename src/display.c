#include "display.h"
#include "scd4x.h"
#include "sps30.h"
#include "pms.h"
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/led.h>
#include <zephyr/input/input.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_DBG);

#if defined(CONFIG_LVGL)

#include <lvgl.h>

const struct led_dt_spec displayled_spec = LED_DT_SPEC_GET(DT_NODELABEL(displayled));
lv_timer_t *led_timer;

lv_obj_t *tile_view;
lv_obj_t *sensor_tile;
lv_obj_t *setting_tile;

lv_obj_t *co2_container;
lv_obj_t *co2_label;
lv_obj_t *pm_container;
lv_obj_t *pm_label;
lv_obj_t *temp_container;
lv_obj_t *temp_label;
lv_obj_t *hum_container;
lv_obj_t *hum_label;

lv_obj_t *wifi_container;
lv_obj_t *wifi_label;

lv_obj_t *lora_container;
lv_obj_t *lora_label;

static scd4x_data_t scd4x_data;
static sps30data_t sps30_data;

void lv_timer(lv_timer_t *timer)
{
	char str[50];
	lv_color_t color = lv_color_hex(0x44ce1b);

	if (0 == get_scd4x_data(&scd4x_data))
	{
		sprintf(str, "CO2\n%i", scd4x_data.co2);
		lv_label_set_text(co2_label, str);
		if (scd4x_data.co2 <= 800)
		{
			color = lv_color_hex(0x44ce1b);
		}
		if (scd4x_data.co2 > 800 && scd4x_data.co2 <= 1200)
		{
			color = lv_color_hex(0xf7e379);
		}
		if (scd4x_data.co2 > 1200)
		{
			color = lv_color_hex(0xe51f1f);
		}
		lv_obj_set_style_bg_color(co2_container, color, LV_PART_MAIN);

		if (scd4x_data.co2 != 0)
		{
			// lv_chart_set_next_value(co2_chart, co2_ser, (int32_t)scd4x_data.co2);

			sprintf(str, "%i.%i°C", (int)scd4x_data.temp, (int)((scd4x_data.temp - (int)scd4x_data.temp) * 10));
			lv_label_set_text(temp_label, str);

			sprintf(str, "%i.%i%%", (int)scd4x_data.hum, (int)((scd4x_data.hum - (int)scd4x_data.hum) * 10));
			lv_label_set_text(hum_label, str);
		}
	}

	if (0 == sps30_get_data(&sps30_data))
	{
		sprintf(str, "PM2.5\n%i\n%i\n%i", (int)sps30_data.pm1, (int)sps30_data.pm25, (int)sps30_data.pm10);
		lv_label_set_text(pm_label, str);

		if (sps30_data.pm25 <= 15)
		{
			color = lv_color_hex(0x44ce1b);
		}
		if (sps30_data.pm25 > 15 && sps30_data.pm25 <= 35)
		{
			color = lv_color_hex(0xf7e379);
		}
		if (sps30_data.pm25 > 35)
		{
			color = lv_color_hex(0xe51f1f);
		}
		lv_obj_set_style_bg_color(pm_container, color, LV_PART_MAIN);
	}
}

void lv_sensor_page()
{
	sensor_tile = lv_tileview_add_tile(tile_view, 0, 0, LV_DIR_RIGHT);

	static lv_style_t sensor_title_style;
	lv_style_init(&sensor_title_style);
	lv_style_set_width(&sensor_title_style, lv_pct(50));
	lv_style_set_height(&sensor_title_style, lv_pct(50));
	lv_style_set_text_font(&sensor_title_style, &lv_font_montserrat_26);

	co2_container = lv_obj_create(sensor_tile);
	lv_obj_add_style(co2_container, &sensor_title_style, 0);
	lv_obj_set_align(co2_container, LV_ALIGN_TOP_LEFT);
	lv_obj_set_style_pad_all(co2_container, 0, LV_STATE_DEFAULT);

	co2_label = lv_label_create(co2_container);
	lv_obj_set_align(co2_label, LV_ALIGN_CENTER);
	lv_label_set_text(co2_label, "CO2\n1233");

	pm_container = lv_obj_create(sensor_tile);
	lv_obj_add_style(pm_container, &sensor_title_style, 0);
	lv_obj_set_align(pm_container, LV_ALIGN_TOP_RIGHT);
	pm_label = lv_label_create(pm_container);
	lv_obj_set_align(pm_label, LV_ALIGN_CENTER);

	static lv_style_t pm_label_style;
	lv_style_init(&pm_label_style);
	lv_style_set_text_font(&pm_label_style, &lv_font_montserrat_22);
	// lv_style_set_text_align(&pm_label_style,LV_TEXT_ALIGN_CENTER);
	lv_obj_add_style(pm_label, &pm_label_style, 0);

	temp_container = lv_obj_create(sensor_tile);
	lv_obj_add_style(temp_container, &sensor_title_style, 0);
	lv_obj_set_align(temp_container, LV_ALIGN_BOTTOM_LEFT);
	temp_label = lv_label_create(temp_container);
	lv_obj_set_align(temp_label, LV_ALIGN_CENTER);
	// lv_label_set_text(temp_label, "Loading");

	hum_container = lv_obj_create(sensor_tile);
	lv_obj_add_style(hum_container, &sensor_title_style, 0);
	lv_obj_set_align(hum_container, LV_ALIGN_BOTTOM_RIGHT);
	hum_label = lv_label_create(hum_container);
	lv_obj_set_align(hum_label, LV_ALIGN_CENTER);
	// lv_label_set_text(hum_label, "Loading");
}

void turn_off_led(lv_timer_t *timer)
{
	led_off_dt(&displayled_spec);
}

void turn_on_led(struct input_event *evt, void *)
{
	led_on_dt(&displayled_spec);
	lv_timer_reset(led_timer);
}

void display_thread(void *, void *, void *)
{
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	display_blanking_off(display_dev);

	tile_view = lv_tileview_create(lv_screen_active());
	lv_sensor_page();

	lv_timer_t *timer = lv_timer_create(lv_timer, 5000, NULL);

	led_on_dt(&displayled_spec);
	led_timer = lv_timer_create(turn_off_led, 15 * 60 * 1000, NULL);
	INPUT_CALLBACK_DEFINE(NULL, turn_on_led, NULL);

	while (1)
	{
		lv_timer_handler();
		k_sleep(K_MSEC(5));
	}
}

K_THREAD_DEFINE(display_thread_tid, 8912,
				display_thread, NULL, NULL, NULL,
				5, 0, 0);

#endif

#if defined(CONFIG_CHARACTER_FRAMEBUFFER)
void cfb_thread(void *, void *, void *)
{
	const struct device *dev;
	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
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
		return;
	}

	cfb_framebuffer_clear(dev, true);
	display_blanking_off(dev);
	cfb_framebuffer_invert(dev);
	cfb_set_kerning(dev, 1);

	int font_number = cfb_get_numof_fonts(dev);

	scd4x_data_t scd4x_data;
	pms_data_t pms_data;
	char str[100];
	while (1)
	{
		cfb_framebuffer_clear(dev, true);
		if (0 == get_scd4x_data(&scd4x_data))
		{
			sprintf(str, "%i", scd4x_data.co2);
			cfb_framebuffer_set_font(dev, font_number - 1);
			cfb_draw_text(dev, str, 0, 0);

			sprintf(str, "%.1fC", scd4x_data.temp);
			cfb_framebuffer_set_font(dev, font_number - 2);
			cfb_draw_text(dev, str, 0, 25);

			sprintf(str, "%.1f%%", scd4x_data.hum);
			cfb_framebuffer_set_font(dev, font_number - 2);
			cfb_draw_text(dev, str, 0, 45);
		}
		if (0 == pms_get(&pms_data))
		{
			sprintf(str, "%i", pms_data.pm1);
			cfb_framebuffer_set_font(dev, font_number - 2);
			cfb_draw_text(dev, str, 90, 0);

			sprintf(str, "%i", pms_data.pm25);
			cfb_framebuffer_set_font(dev, font_number - 2);
			cfb_draw_text(dev, str, 90, 22);

			sprintf(str, "%i", pms_data.pm10);
			cfb_framebuffer_set_font(dev, font_number - 2);
			cfb_draw_text(dev, str, 90, 44);
		}

		cfb_framebuffer_finalize(dev);
		k_msleep(5000);
	}
}

K_THREAD_DEFINE(cfb_thread_tid, 8912,
				cfb_thread, NULL, NULL, NULL,
				5, 0, 0);

#endif