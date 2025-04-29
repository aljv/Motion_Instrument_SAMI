#pragma once

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#define SPIOP	SPI_WORD_SET(8) | SPI_TRANSFER_MSB

int app_spi_is_ready(const struct spi_dt_spec *spec);
int app_spi_transceive(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_buf, const struct spi_buf_set *rx_bufs);