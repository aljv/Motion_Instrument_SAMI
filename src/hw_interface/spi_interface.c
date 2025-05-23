#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "spi_interface.h"

LOG_MODULE_REGISTER(spi_interface, LOG_LEVEL_DBG);

int app_spi_is_ready(const struct spi_dt_spec *spec)
{
    // Fix: spi_is_ready_dt returns true (1) when ready, false (0) when not ready
    bool ready = spi_is_ready_dt(spec);
    if (!ready) {
        LOG_ERR("Error: SPI device is not ready");
        return -ENODEV;
    }
    
    LOG_DBG("SPI device is ready");
    return 0;
}

int app_spi_transceive(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_buf, const struct spi_buf_set *rx_bufs)
{
    // Fix: spi_transceive_dt returns 0 on success, negative on error
    int err = spi_transceive_dt(spec, tx_buf, rx_bufs);
    if (err) {
        LOG_ERR("spi_transceive_dt() failed, err: %d", err);
        return err;
    }
    
    LOG_DBG("SPI transceive successful");
    return 0;
}

int app_spi_write(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_buf)
{
    // Write-only operation (no receive buffer)
    int err = spi_write_dt(spec, tx_buf);
    if (err) {
        LOG_ERR("spi_write_dt() failed, err: %d", err);
        return err;
    }
    
    LOG_DBG("SPI write successful");
    return 0;
}