#include "dog.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

#define WDT_FEED_TRIES 5

static uint8_t dog_run = 0;

void dog_stop()
{
    dog_run = 1;
}

void dog_thread(void *, void *, void *)
{
    int err;
    int wdt_channel_id;
    const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

    printk("Watchdog sample application\n");

    if (!device_is_ready(wdt))
    {
        printk("%s: device not ready.\n", wdt->name);
        return ;
    }

    struct wdt_timeout_cfg wdt_config = {
        /* Reset SoC when watchdog timer expires. */
        .flags = WDT_FLAG_RESET_SOC,

        /* Expire watchdog after max window */
        .window.min = 0,
        .window.max = 30000,
    };

    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    if (wdt_channel_id == -ENOTSUP)
    {
        /* IWDG driver for STM32 doesn't support callback */
        printk("Callback support rejected, continuing anyway\n");
        wdt_config.callback = NULL;
        wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    }
    if (wdt_channel_id < 0)
    {
        printk("Watchdog install error\n");
        return;
    }

    err = wdt_setup(wdt, 0);
    if (err < 0)
    {
        printk("Watchdog setup error\n");
        return ;
    }

    while (1)
    {
        if (dog_run == 0)
        {
            // printk("Feeding watchdog...\n");
            wdt_feed(wdt, wdt_channel_id);
        }

        k_sleep(K_MSEC(15000));
    }
}

K_THREAD_DEFINE(dog_tid, 2000,
                dog_thread, NULL, NULL, NULL,
                5, 0, 0);