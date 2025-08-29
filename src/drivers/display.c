#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include "display.h"
#include "sensors.h"
#include <zephyr/display/cfb.h>

#ifdef CONFIG_DISPLAY

LOG_MODULE_REGISTER(display, LOG_LEVEL_ERR);

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

void display_thread(void *, void *, void *)
{
    if (!device_is_ready(dev))
    {
        LOG_ERR("Device not ready, aborting test");
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

    if (cfb_framebuffer_init(dev))
    {
        printf("Framebuffer initialization failed!\n");
        return;
    }

    cfb_framebuffer_set_font(dev, 1);
    cfb_framebuffer_clear(dev, true);
    cfb_print(dev, "Init...", 0, 0);
    cfb_framebuffer_finalize(dev);
    display_blanking_off(dev);

    uint16_t co2;
    uint16_t pm1, pm25, pm10;
    float temp, hum;
    char str[100];

    while (1)
    {
        k_msleep(30 * 1000);

        sensor_get_co2(&co2);
        sensor_get_pm1(&pm1);
        sensor_get_pm25(&pm25);
        sensor_get_pm10(&pm10);
        sensor_get_temp(&temp);
        sensor_get_hum(&hum);

        cfb_framebuffer_clear(dev, false);

        cfb_framebuffer_set_font(dev, 1);
        sprintf(str, "%u", co2);
        cfb_draw_text(dev, str, 0, 0);

        sprintf(str, "%u %u %u", pm1, pm25, pm10);
        cfb_draw_text(dev, str, 0, 22);

        cfb_framebuffer_set_font(dev, 0);
        sprintf(str, "%.1fC %.1f%%", temp, hum);
        cfb_draw_text(dev, str, 0, 46);

        cfb_framebuffer_finalize(dev);
    }
}

K_THREAD_DEFINE(display_pid, 4096,
                display_thread, NULL, NULL, NULL,
                7, 0, 2000);

#endif