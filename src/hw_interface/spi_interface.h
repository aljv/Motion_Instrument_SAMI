#ifndef SPI_INTERFACE_H
#define SPI_INTERFACE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#define SPIOP	SPI_WORD_SET(8) | SPI_TRANSFER_MSB

/**
 * @brief Check if SPI device is ready
 * 
 * @param spec SPI device tree spec
 * @return int 0 on success, negative error code otherwise
 */
int app_spi_is_ready(const struct spi_dt_spec *spec);

/**
 * @brief Perform SPI transceive operation
 * 
 * @param spec SPI device tree spec
 * @param tx_buf Transmit buffer set
 * @param rx_bufs Receive buffer set (can be NULL for write-only)
 * @return int 0 on success, negative error code otherwise
 */
int app_spi_transceive(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_buf, const struct spi_buf_set *rx_bufs);

/**
 * @brief Perform SPI write operation
 * 
 * @param spec SPI device tree spec
 * @param tx_buf Transmit buffer set
 * @return int 0 on success, negative error code otherwise
 */
int app_spi_write(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_buf);

/**
 * @brief Perform SPI read operation
 * 
 * @param spec SPI device tree spec
 * @param rx_buf receiver buffer set
 * @return int 0 on success, negative error code otherwise
 */
int app_spi_read(const struct spi_dt_spec *spec, const struct spi_buf_set *rx_buf);

#endif /* SPI_INTERFACE_H */