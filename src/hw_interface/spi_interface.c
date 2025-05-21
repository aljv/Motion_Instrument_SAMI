#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "spi_interface.h"

#define MODULE spi_interface
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);


int app_spi_is_ready(const struct spi_dt_spec *spec)
{
        int err = spi_is_ready_dt(&spec);
        if (!err) 
        {
	        LOG_ERR("Error: SPI device is not ready, err: %d", err);
	        return err;
        }
        else
        {
                return 0;
        }
        
}

int app_spi_transceive(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_buf, const struct spi_buf_set *rx_bufs)
{
        int err = spi_transceive_dt(&spec, &tx_buf, &rx_bufs);
        if (!err) 
        {
	        LOG_ERR("spi_transceive_dt() failed, err: %d", err);
	        return err;
        }
        else
        {
                return 0;
        }
}