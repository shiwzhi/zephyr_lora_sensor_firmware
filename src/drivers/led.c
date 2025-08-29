#include "led.h"
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

uint16_t _systemled_delay = 0;

void led_toggle_system(uint16_t ms)
{
    _systemled_delay = ms;
}

#if DT_HAS_ALIAS(systemled)

static void led_thread(void *, void *, void *)
{
    const struct led_dt_spec systemled = LED_DT_SPEC_GET(DT_ALIAS(systemled));

    while (1)
    {
        if (_systemled_delay == 0)
        {
            led_off_dt(&systemled);
            k_msleep(1000);
        }
        else
        {
            led_on_dt(&systemled);
            k_msleep(_systemled_delay);
            led_off_dt(&systemled);
            k_msleep(_systemled_delay);
        }
    }
}

K_THREAD_DEFINE(led_tid, 1024,
                led_thread, NULL, NULL, NULL,
                7, 0, 0);
#endif
