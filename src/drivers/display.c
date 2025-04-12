#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include "display.h"
#include "sensors.h"

LOG_MODULE_REGISTER(display, LOG_LEVEL_ERR);

#ifdef CONFIG_LVGL

#include <lvgl.h>

static bool display_is_running = false;
static const struct device *display_dev;

void display_thread(void *, void *, void *)
{
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev))
    {
        LOG_ERR("Device not ready, aborting test");
        return;
    }
    display_blanking_off(display_dev);
    display_is_running = true;
    LOG_INF("display ready");

    lv_obj_t *co2_chart;
    co2_chart = lv_chart_create(lv_screen_active());
    lv_obj_set_size(co2_chart, lv_pct(50), lv_pct(50));
    lv_obj_align(co2_chart, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_chart_set_type(co2_chart, LV_CHART_TYPE_LINE);
    lv_chart_series_t *co2_ser = lv_chart_add_series(co2_chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(co2_chart, LV_CHART_AXIS_PRIMARY_Y, 400, 1500);
    lv_obj_set_style_size(co2_chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(co2_chart, 1, LV_PART_INDICATOR);
    lv_chart_set_point_count(co2_chart, 720);

    lv_obj_t *co2_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(co2_label, &lv_font_montserrat_16, 0);
    lv_obj_align(co2_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *pm_chart = lv_chart_create(lv_screen_active());
    lv_obj_set_size(pm_chart, lv_pct(50), lv_pct(50));
    lv_obj_align(pm_chart, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_chart_set_type(pm_chart, LV_CHART_TYPE_LINE);
    lv_chart_series_t *pm_ser = lv_chart_add_series(pm_chart, lv_palette_main(LV_PALETTE_GREY), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(pm_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_size(pm_chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(pm_chart, 1, LV_PART_INDICATOR);
    lv_chart_set_point_count(pm_chart, 720);

    lv_obj_t *pm_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(pm_label, &lv_font_montserrat_16, 0);
    lv_obj_align(pm_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_timer_handler();

    while (1)
    {
        k_msleep(30 * 1000);

        uint16_t co2;
        if (sensor_get_co2(&co2) == 0)
        {
            lv_chart_set_next_value(co2_chart, co2_ser, (int32_t)co2);
            lv_chart_refresh(co2_chart);
            lv_label_set_text_fmt(co2_label, "%d ppm", co2);
        }

        uint16_t pm25, pm1, pm10;
        if (sensor_get_pm25(&pm25) == 0)
        {
            lv_chart_set_next_value(pm_chart, pm_ser, (int32_t)pm25);
            lv_chart_refresh(pm_chart);

            sensor_get_pm1(&pm1);
            sensor_get_pm10(&pm10);
            lv_label_set_text_fmt(pm_label, "%d %d %d", pm1, pm25, pm10);
        }

        lv_timer_handler();
    }
}

K_THREAD_DEFINE(display_pid, 4096,
                display_thread, NULL, NULL, NULL,
                7, 0, 0);

#endif