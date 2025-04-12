#include "sx126x_hal.h"
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(SX126X_HAL, LOG_LEVEL_DBG);

typedef enum
{
    RADIO_SLEEP,
    RADIO_AWAKE
} radio_sleep_mode_t;

static radio_sleep_mode_t radio_mode = RADIO_AWAKE;

static struct spi_dt_spec spispec = SPI_DT_SPEC_GET(DT_ALIAS(sx126x_spi), SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);
static const struct gpio_dt_spec nrst = GPIO_DT_SPEC_GET(DT_ALIAS(nrstpin), gpios);
static const struct gpio_dt_spec busy = GPIO_DT_SPEC_GET(DT_ALIAS(busypin), gpios);
static const struct gpio_dt_spec nss = SPI_CS_GPIOS_DT_SPEC_GET(DT_ALIAS(sx126x_spi));

void sx126x_hal_check_device_ready()
{
    // LOG_DBG("Check busy");
    if (radio_mode != RADIO_SLEEP)
    {
        while (gpio_pin_get_dt(&busy) == 1)
        {
        };
    }
    else
    {
        gpio_pin_set_dt(&nss, 1);
        while (gpio_pin_get_dt(&busy) == 1)
        {
        };
        gpio_pin_set_dt(&nss, 0);
        radio_mode = RADIO_AWAKE;
    }
}

sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command, const uint16_t command_length,
                                     const uint8_t *data, const uint16_t data_length)
{
    // LOG_DBG("Write");
    sx126x_hal_check_device_ready();

    int err = spi_is_ready_dt(&spispec);
    if (!err)
    {
        LOG_ERR("Error: SPI device is not ready, err: %d", err);
        return SX126X_HAL_STATUS_ERROR;
    }

    uint8_t tx_buffer[200];
    memcpy(tx_buffer, command, command_length);
    memcpy(&tx_buffer[command_length], data, data_length);

    LOG_HEXDUMP_DBG(tx_buffer, command_length + data_length, "Write: TX: ");

    struct spi_buf command_buf = {.buf = tx_buffer, .len = command_length + data_length};
    struct spi_buf_set tx_spi_buf_set = {.buffers = &command_buf, .count = 1};

    err = spi_transceive_dt(&spispec, &tx_spi_buf_set, NULL);
    if (err < 0)
    {
        LOG_ERR("spi_transceive_dt() failed, err: %d", err);
        return SX126X_HAL_STATUS_ERROR;
    }

    if (command[0] != 0x84)
    {
        sx126x_hal_check_device_ready();
    }
    else
    {
        LOG_DBG("Change to sleep");
        radio_mode = RADIO_SLEEP;
    }

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command, const uint16_t command_length,
                                    uint8_t *data, const uint16_t data_length)
{
    // LOG_DBG("Read");

    sx126x_hal_check_device_ready();

    int err = spi_is_ready_dt(&spispec);
    if (!err)
    {
        LOG_ERR("Error: SPI device is not ready, err: %d", err);
        return SX126X_HAL_STATUS_ERROR;
    }

    uint8_t tx_buffer[200];
    memcpy(tx_buffer, command, command_length);
    memcpy(&tx_buffer[command_length], data, data_length);

    LOG_HEXDUMP_DBG(tx_buffer, command_length + data_length, "Read TX: ");

    struct spi_buf spi_data_tx_buf = {.buf = tx_buffer, .len = command_length + data_length};
    struct spi_buf_set spi_tx_buf_set = {.buffers = &spi_data_tx_buf, .count = 1};

    uint8_t rx_buffer[200];
    struct spi_buf spi_data_rx_buf = {.buf = rx_buffer, .len = command_length + data_length};
    struct spi_buf_set rx_spi_buf_set = {.buffers = &spi_data_rx_buf, .count = 1};

    err = spi_transceive_dt(&spispec, &spi_tx_buf_set, &rx_spi_buf_set);
    if (err < 0)
    {
        LOG_ERR("spi_transceive_dt() failed, err: %d", err);
        return SX126X_HAL_STATUS_ERROR;
    }

    memcpy(data, &rx_buffer[command_length], data_length);

    sx126x_hal_check_device_ready();

    // LOG_HEXDUMP_DBG(rx_buffer, command_length + data_length, "Read RX buffer: ");
    LOG_HEXDUMP_DBG(data, data_length, "Read RX data: ");

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
    LOG_DBG("SX1262 hal reset");
    if (!gpio_is_ready_dt(&nrst))
    {
        LOG_ERR("NRST not ready");
        return SX126X_HAL_STATUS_ERROR;
    }
    gpio_pin_configure_dt(&nrst, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&busy, GPIO_INPUT);

    gpio_pin_set_dt(&nrst, 1);
    k_msleep(1);
    gpio_pin_set_dt(&nrst, 0);
    k_msleep(1);
    radio_mode = RADIO_AWAKE;
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void *context)
{
    LOG_DBG("SX1262 hal wakeup");

    sx126x_hal_check_device_ready();

    return SX126X_HAL_STATUS_OK;
}