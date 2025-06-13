#ifndef VS1053_INTERFACE_H
#define VS1053_INTERFACE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

// for controlling SPI transfer between MCU and CODEC
typedef enum {
    SPI_DATA,
    SPI_CTRL
} spi_xfer_type_t;

// VS1053 Function prototypes
void VS1053Init(void);
void VS1053UpdateVolume(int8_t volumeL, int8_t volumeR);
void VS1053WriteSci(uint8_t addr, uint16_t data);
uint16_t VS1053ReadSci(uint8_t addr);
int VS1053WriteSdi(const uint8_t *data, uint8_t len);
void VS1053WriteMem(uint16_t addr, uint16_t data);
uint16_t VS1053ReadMem(uint16_t addr);
uint8_t VS1053HardwareReset(void);
uint8_t VS1053SoftwareReset(void);
void VS1053bLoadPlugin(const uint16_t *data, int len);
uint16_t VS1053ReadSci_Debug(uint8_t addr);
void VS1053TestSPI(void);
void vs1053_register_test_suite(void);
void setup_vs1053_midi_mode(void);

void app_spi_xfer(spi_xfer_type_t type, uint8_t* tx_dat, uint8_t* rx_dat, uint8_t len);

#endif // VS1053_INTERFACE_H