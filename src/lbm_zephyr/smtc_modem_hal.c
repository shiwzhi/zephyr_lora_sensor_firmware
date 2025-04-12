#include "smtc_modem_hal.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(SMTC_MODEM_HAL, LOG_LEVEL_DBG);

static const struct gpio_dt_spec dio1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(dio1pin), gpios, {0});
static bool is_modem_irq = true;
static struct gpio_callback dio1_cb_data;
static void (*dio1callback)(void *context) = NULL;
static void *irqcontext = NULL;
static void (*smtc_timer_callback)(void *context) = NULL;
static void *smtc_timer_context = NULL;
static bool is_timer_irq_pending = false;

uint32_t smtc_modem_hal_get_time_in_ms(void)
{
    return k_uptime_get();
}

uint32_t smtc_modem_hal_get_time_in_s(void)
{
    return k_uptime_get() / 1000;
}

void smtc_modem_hal_start_radio_tcxo(void)
{
    return;
}

void smtc_modem_hal_stop_radio_tcxo(void)
{
    return;
}

uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
    return 0;
}

void smtc_modem_hal_set_ant_switch(bool is_tx_on)
{
    // todo
    return;
}

static void dio1_triggered(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (is_modem_irq)
    {
        LOG_DBG("dio callback");

        dio1callback(irqcontext);
    }
}

void smtc_modem_hal_irq_config_radio_irq(void (*callback)(void *context), void *context)
{
    LOG_DBG("CONFIG radio IRQ");
    dio1callback = callback;
    irqcontext = context;

    int ret;

    if (!gpio_is_ready_dt(&dio1))
    {
        LOG_ERR("dio1 pin not ready");
        return;
    }

    ret = gpio_pin_configure_dt(&dio1, GPIO_INPUT);
    if (ret != 0)
    {
        LOG_ERR("Failed to config dio1 pin as input");
        return;
    }

    ret = gpio_pin_interrupt_configure_dt(&dio1, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to config dio1 interrupt");
        return;
    }

    gpio_init_callback(&dio1_cb_data, dio1_triggered, BIT(dio1.pin));
    gpio_add_callback(dio1.port, &dio1_cb_data);
}

void timer_callback(struct k_timer *timer_id)
{
    if (is_modem_irq)
    {
        LOG_DBG("timer callback");

        smtc_timer_callback(smtc_timer_context);
    }
    else
    {
        LOG_DBG("timer pending");

        is_timer_irq_pending = true;
    }
}
K_TIMER_DEFINE(smtc_timer, timer_callback, NULL);

void smtc_modem_hal_start_timer(const uint32_t milliseconds, void (*callback)(void *context), void *context)
{
    LOG_DBG("start timer: %u ms", milliseconds);

    smtc_timer_callback = callback;
    smtc_timer_context = context;

    k_timer_start(&smtc_timer, K_MSEC(milliseconds), K_NO_WAIT);
}

void smtc_modem_hal_stop_timer(void)
{
    k_timer_stop(&smtc_timer);
}

void smtc_modem_hal_disable_modem_irq(void)
{
    LOG_DBG("disable irq");
    is_modem_irq = false;
}

void smtc_modem_hal_enable_modem_irq(void)
{
    LOG_DBG("enable irq");
    is_modem_irq = true;
    if (is_timer_irq_pending)
    {
        is_timer_irq_pending = false;
        smtc_timer_callback(smtc_timer_context);
    }
}

uint32_t smtc_modem_hal_get_random_nb_in_range(const uint32_t val_1, const uint32_t val_2)
{
    uint32_t min = MIN(val_1, val_2);
    uint32_t max = MAX(val_1, val_2);
    uint32_t range = (max - min + 1);
    range = range ? range : UINT32_MAX;
    return (uint32_t)((sys_rand32_get() % range) + min);
}

void smtc_modem_hal_print_trace(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char string[255];
    if (0 < vsprintf(string, fmt, args)) // build string
    {
        LOG_INF("%s", string);
    }
    va_end(args);
}

void smtc_modem_hal_on_panic(uint8_t *func, uint32_t line, const char *fmt, ...)
{
    uint8_t out_buff[255] = {0};
    uint8_t out_len = snprintf((char *)out_buff, sizeof(out_buff), "%s:%u ", func, line);

    va_list args;
    va_start(args, fmt);
    out_len += vsprintf((char *)&out_buff[out_len], fmt, args);
    va_end(args);

    smtc_modem_hal_crashlog_store(out_buff, out_len);

    // SMTC_HAL_TRACE_ERROR("Modem panic: %s\n", out_buff);
}

bool smtc_modem_hal_crashlog_get_status(void)
{
    return false;
}

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset, uint8_t *buffer,
                                    const uint32_t size)
{
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset, const uint8_t *buffer,
                                  const uint32_t size)
{
}

void smtc_modem_hal_user_lbm_irq(void)
{
}

void smtc_modem_hal_reset_mcu(void)
{
}

int8_t smtc_modem_hal_get_board_delay_ms(void)
{
    return 1;
}

uint8_t smtc_modem_hal_get_battery_level(void)
{
    return 255;
}

void smtc_modem_hal_crashlog_store(const uint8_t *crash_string, uint8_t crash_string_length)
{
}