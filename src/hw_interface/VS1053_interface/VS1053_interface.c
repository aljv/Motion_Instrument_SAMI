#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include "VS1053_interface.h"
#include "VS10xx_uc.h"
#include "spi_interface.h"

// Include plugin data
#include "rtmidistart.plg"

#define MODULE vs1053_interface
LOG_MODULE_REGISTER(MODULE);

// Define pin numbers directly from the schematic
#define VS_PIN_DREQ      4  // P1.04
#define VS_PIN_RESET     6  // P1.06
#define VS_PIN_XCS       7  // P1.07
#define VS_PIN_XDCS      5  // P0.05


// SPI commands
#define SCI_READ_FLAG 0x03
#define SCI_WRITE_FLAG 0x02
#define SDI_MAX_PACKET_LEN 256

// Node labels
#define VS_SPI_DEVICE DT_NODELABEL(vs1053_spi)
#define VS_DREQ_PIN DT_ALIAS(vsdreq)
#define VS_RESET_PIN DT_ALIAS(vsreset)
#define VS_MCS_PIN DT_ALIAS(vsmcs)
#define VS_DCS_PIN DT_ALIAS(vsdcs)

// Define GPIO specs - these must be static const and initialized at compile time
static const struct gpio_dt_spec vs_gpio_dreq = GPIO_DT_SPEC_GET(VS_DREQ_PIN, gpios);
static const struct gpio_dt_spec vs_gpio_reset = GPIO_DT_SPEC_GET(VS_RESET_PIN, gpios);
static const struct gpio_dt_spec vs_gpio_mcs = GPIO_DT_SPEC_GET(VS_MCS_PIN, gpios);
static const struct gpio_dt_spec vs_gpio_dcs = GPIO_DT_SPEC_GET(VS_DCS_PIN, gpios);

// Define SPI spec
struct spi_dt_spec vs_spi_dev = SPI_DT_SPEC_GET(VS_SPI_DEVICE, SPIOP, 0);

// VS1053 variables
const uint16_t chipNumber[16] = {1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103, 0, 0, 0, 0, 0, 0, 0, 0};
#define VS1053_XFER_LEN_B 4

//< VS1053 Serial Control Interface Write
/*
* @brief
* writes @param data to register at @param addr in VS1053
*/
void VS1053WriteSci(uint8_t addr, uint16_t data) {
    // Wait for DREQ to go high (chip is ready)
    while(gpio_pin_get_dt(&vs_gpio_dreq) != 1);
    
    // Activate the SCI CS pin (active low)
    gpio_pin_set_dt(&vs_gpio_mcs, 0);
    
    uint8_t tx_buf[VS1053_XFER_LEN_B] = {
        SCI_WRITE_FLAG, 
        addr, 
        (uint8_t)(data >> 8), 
        (uint8_t)(data & 0xFF)
    };
    
    struct spi_buf tx_bufs = {
        .buf = tx_buf,
        .len = VS1053_XFER_LEN_B
    };
    
    struct spi_buf_set tx = {
        .buffers = &tx_bufs,
        .count = 1
    };
    
    int ret = app_spi_write(&vs_spi_dev, &tx);
    if (ret != 0) {
        LOG_ERR("SCI write failed: %d", ret);
    }
    
    // Deactivate the SCI CS pin
    gpio_pin_set_dt(&vs_gpio_mcs, 1);
}

//< VS1053 Serial Control Interface Read
/*
* @brief
* reads contents of register at @param addr in VS1053
*/
uint16_t VS1053ReadSci(uint8_t addr) {
    // Wait for DREQ to go high (chip is ready)
    while(gpio_pin_get_dt(&vs_gpio_dreq)!= 1);
    
    uint16_t res;
    uint8_t tx_buf[VS1053_XFER_LEN_B] = {SCI_READ_FLAG, addr, 0, 0};
    uint8_t rx_buf[VS1053_XFER_LEN_B] = {0};
  
    // Activate the SCI CS pin (active low)
    gpio_pin_set_dt(&vs_gpio_mcs, 0);
    
    struct spi_buf tx_bufs = {
        .buf = tx_buf,
        .len = VS1053_XFER_LEN_B
    };
    
    struct spi_buf rx_bufs = {
        .buf = rx_buf,
        .len = VS1053_XFER_LEN_B
    };
    
    struct spi_buf_set tx = {
        .buffers = &tx_bufs,
        .count = 1
    };
    
    struct spi_buf_set rx = {
        .buffers = &rx_bufs,
        .count = 1
    };
    
    int ret = app_spi_transceive(&vs_spi_dev, &tx, &rx);
    if (ret != 0) {
        LOG_ERR("SCI read failed: %d", ret);
        return 0;
    }
    
    // Deactivate the SCI CS pin
    gpio_pin_set_dt(&vs_gpio_mcs, 1);

    res = (rx_buf[2] << 8); //load upper byte
    res += (rx_buf[3]); //load lower byte
    return res;
}

//< VS1053 Serial Data Interface Write
/*
* @brief
* writes @param data of @param len direct to memory in VS1053
*/
int VS1053WriteSdi(const uint8_t *data, uint8_t len) {
    if (len > SDI_MAX_PACKET_LEN)
        return -1;
        
    // Wait for DREQ to go high (chip is ready)
    while(gpio_pin_get_dt(&vs_gpio_dreq) != 1);
    
    // Activate the SDI CS pin (active low)
    gpio_pin_set_dt(&vs_gpio_dcs, 0);
    
    struct spi_buf tx_bufs = {
        .buf = (void *)data,
        .len = len
    };
    
    struct spi_buf_set tx = {
        .buffers = &tx_bufs,
        .count = 1
    };
    
    int ret = app_spi_write(&vs_spi_dev, &tx);
    if (ret != 0) {
        LOG_ERR("SDI write failed: %d", ret);
        return -1;
    }
    
    // Deactivate the SDI CS pin
    gpio_pin_set_dt(&vs_gpio_dcs, 1);

    return 0;
}

/**< miscellaneous - begin >**/

//< VS1053 load plugin
/*
* @brief
* loads @global plugin as @param data of @param len direct to memory in VS1053
*/
void VS1053bLoadPlugin(const uint16_t *data, int len) {
    int i = 0;
    unsigned short addr, n, val;
    while (i<len) {
        addr = data[i++];
        n = data[i++];
        if (n & 0x8000U) {
            n &= 0x7FFF;
            val = data[i++];
            while (n--)
                VS1053WriteSci(addr, val);
        } else {
            while (n--) {
                val = data[i++];
                VS1053WriteSci(addr, val);
            }
        }
    }
}

/**< miscellaneous - end >**/

/**< memory access - begin >**/

//< VS1053 memory write
/*
* @brief
* writes @param data at memory location @param addr in VS1053
*/
void VS1053WriteMem(uint16_t addr, uint16_t data) {
    VS1053WriteSci(SCI_WRAMADDR, addr);
    VS1053WriteSci(SCI_WRAM, data);
}

//< VS1053 memory read
/*
* @brief
* reads contents at memory location @param addr in VS1053
*/
uint16_t VS1053ReadMem(uint16_t addr) {
    VS1053WriteSci(SCI_WRAMADDR, addr);
    return VS1053ReadSci(SCI_WRAM);
}

/**< memory access - end >**/


/**< Init and reset - begin >**/

//< VS1053 hardware reset
/*
* @brief
* hardware resets VS1053
* @note
* Generates speaker pop when RIGHT/LIGHT analog output channels are brought low by reset
* (fix) remove filter cap / lower value on RIGHT/LEFT then mute max while lines are low
*/
uint8_t VS1053HardwareReset(void) {
    // Pull reset pin low
    gpio_pin_set_dt(&vs_gpio_reset, 0);
    k_msleep(10);
    
    // Release reset pin
    gpio_pin_set_dt(&vs_gpio_reset, 1);
    k_msleep(10);

    return 1;
}

//< VS1053 software reset
/*
* @brief
* software resets VS1053
*/
uint8_t VS1053SoftwareReset(void) {
    VS1053WriteSci(SCI_MODE, SM_SDINEW | SM_SDISHARE | SM_TESTS | SM_RESET);
    VS1053WriteSci(SCI_CLOCKF, 0xC000);

    VS1053WriteSci(SCI_AICTRL1, 0xABAD);
    VS1053WriteSci(SCI_AICTRL2, 0x7E57);
    if (VS1053ReadSci(SCI_AICTRL1) != 0xABAD || VS1053ReadSci(SCI_AICTRL2) != 0x7E57) {
        LOG_ERR("There is something wrong with VS10xx SCI registers");
    }
    VS1053WriteSci(SCI_AICTRL1, 65534);               //originally at 0 - vs datasheet pg 54
    VS1053WriteSci(SCI_AICTRL2, 0);                                  

    uint16_t ssVer = ((VS1053ReadSci(SCI_STATUS) >> 4) & 15);
    if (chipNumber[ssVer]) {
        if (chipNumber[ssVer] != 1053) {
            LOG_ERR("Incorrect chip");
            return -1;
        } else 
            LOG_INF("Correct chip"); 
    }

    VS1053bLoadPlugin(plugin, sizeof(plugin)/sizeof(plugin[0]));

    return 1;
}

/* 
Update output volume
"The most significant byte of the volume register controls 
the left channel volume, the low part controls the right channel 
volume. The channel volume sets the attenuation from the maximum 
volume level in 0.5 dB steps. Thus, maximum volume is 0x0000 and 
total silence is 0xFEFE
*/
void VS1053UpdateVolume(int8_t volumeL, int8_t volumeR)
{
  int16_t combinedVol = (volumeR|(volumeL<<8));
  VS1053WriteSci(SCI_VOL, combinedVol);
}

/*
* @brief
* hardware and software resets VS1053
*/
void VS1053Init(void) {
    int ret;
    
    // Get GPIO device

    if (!gpio_is_ready_dt(&vs_gpio_dreq) && !gpio_is_ready_dt(&vs_gpio_reset) &&
        !gpio_is_ready_dt(&vs_gpio_mcs) && !gpio_is_ready_dt(&vs_gpio_dcs)) 
    {
        LOG_ERR("1 or more GPIO devices not found");
        return;
    }


    
    // Get SPI device
    app_spi_is_ready(&vs_spi_dev);
    
    
    // Configure pins
    
    // Configure reset pins
    ret = gpio_pin_configure_dt(&vs_gpio_dreq, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error configuring dreq pin: %d", ret);
        return;
    }

    ret = gpio_pin_configure_dt(&vs_gpio_reset, GPIO_OUTPUT_HIGH);
    if (ret != 0) {
        LOG_ERR("Error configuring reset pin: %d", ret);
        return;
    }

    ret = gpio_pin_configure_dt(&vs_gpio_mcs, GPIO_OUTPUT_HIGH);
    if (ret != 0) {
        LOG_ERR("Error configuring mcs pin: %d", ret);
        return;
    }

    ret = gpio_pin_configure_dt(&vs_gpio_dcs, GPIO_OUTPUT_HIGH);
    if (ret != 0) {
        LOG_ERR("Error configuring dcs pin: %d", ret);
        return;
    }
    

    // Reset and initialize the VS1053
    VS1053HardwareReset();
    VS1053SoftwareReset();
}