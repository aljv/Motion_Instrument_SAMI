#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include "VS1053_interface.h"
#include "VS10xx_uc.h"
#include "spi_interface.h"
#include "i2c_interface.h"

// Include plugin data
#include "rtmidistart.plg"

#define MODULE vs1053_interface
LOG_MODULE_REGISTER(MODULE);

// Define pin numbers directly from the schematic
#define VS_PIN_DREQ      4  // P1.04
#define VS_PIN_RESET     6  // P1.06
#define VS_PIN_XCS       5  // P1.07
#define VS_PIN_XDCS      7  // P0.05


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

#define VS_MCS_CHIP_SELECT_FLAG 0
#define VS_DCS_CHIP_SELECT_FLAG 1

// Define GPIO specs - these must be static const and initialized at compile time
/*static const struct gpio_dt_spec vs_gpio_dreq = GPIO_DT_SPEC_GET(VS_DREQ_PIN, gpios);
static const struct gpio_dt_spec vs_gpio_reset = GPIO_DT_SPEC_GET(VS_RESET_PIN, gpios);
static const struct gpio_dt_spec vs_gpio_mcs = GPIO_DT_SPEC_GET(VS_MCS_PIN, gpios);
static const struct gpio_dt_spec vs_gpio_dcs = GPIO_DT_SPEC_GET(VS_DCS_PIN, gpios);*/

static const struct gpio_dt_spec vs_gpio_mcs = GPIO_DT_SPEC_GET(DT_NODELABEL(vs_mcs), gpios);
static const struct gpio_dt_spec vs_gpio_dcs = GPIO_DT_SPEC_GET(DT_NODELABEL(vs_dcs), gpios);
static const struct gpio_dt_spec vs_gpio_dreq = GPIO_DT_SPEC_GET(DT_NODELABEL(vs_dreq), gpios);
static const struct gpio_dt_spec vs_gpio_reset = GPIO_DT_SPEC_GET(DT_NODELABEL(vs_reset), gpios);

// Define SPI spec
struct spi_dt_spec vs_spi_dev = SPI_DT_SPEC_GET(VS_SPI_DEVICE, SPIOP, 0);

// VS1053 variables
const uint16_t chipNumber[16] = {1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103, 0, 0, 0, 0, 0, 0, 0, 0};
#define VS1053_XFER_LEN_B 4

// Global SPI completion flag
static volatile bool spi_xfer_done = false;

// for debugging in function
static bool debug = false;

// SPI callback function
void spi_callback(const struct device *dev, int result, void *data)
{
    spi_xfer_done = true;
}

void debug_pin_states(const char* context) {
    LOG_INF("=== Pin States: %s ===", context);
    LOG_INF("RESET: %d (should be HIGH when not resetting)", gpio_pin_get_dt(&vs_gpio_reset));
    LOG_INF("DREQ:  %d (should be HIGH when ready)", gpio_pin_get_dt(&vs_gpio_dreq));
    LOG_INF("MCS:   %d (should be HIGH when SCI idle)", gpio_pin_get_dt(&vs_gpio_mcs));
    LOG_INF("DCS:   %d (should be HIGH when SDI idle)", gpio_pin_get_dt(&vs_gpio_dcs));
    
    // Also check the raw pin values to see if there's an inversion issue
    LOG_INF("Raw pin readings:");
    LOG_INF("MCS raw:   %d", gpio_pin_get_raw(vs_gpio_mcs.port, vs_gpio_mcs.pin));
    LOG_INF("DCS raw:   %d", gpio_pin_get_raw(vs_gpio_dcs.port, vs_gpio_dcs.pin));
}


void app_spi_xfer(spi_xfer_type_t type, uint8_t* tx_dat, uint8_t* rx_dat, uint8_t len)
{
    int err;
    
    // Prepare SPI buffers
    struct spi_buf tx_buf = {.buf = tx_dat, .len = len};
    struct spi_buf_set tx_buf_set = {.buffers = &tx_buf, .count = 1};

    struct spi_buf rx_buf = {.buf = rx_dat, .len = len};
    struct spi_buf_set rx_buf_set = {.buffers = &rx_buf, .count = 1};
    
    // Assert correct chip select BEFORE transfer
    if(type == SPI_DATA) {
        gpio_pin_set_dt(&vs_gpio_dcs, 0);  // Assert DCS (active low)
    } else if(type == SPI_CTRL) {
        gpio_pin_set_dt(&vs_gpio_mcs, 0);  // Assert MCS
    } else {
        LOG_ERR("Invalid SPI transfer type");
        err = -EINVAL;
        return;
    }
    
    // Small delay to ensure CS setup time
    k_usleep(1);
    
    // Perform SPI transfer
    if(tx_dat != NULL && rx_dat != NULL) {
        err = app_spi_transceive(&vs_spi_dev, &tx_buf_set, &rx_buf_set);
    } else if(tx_dat != NULL) {
        err = app_spi_write(&vs_spi_dev, &tx_buf_set);
    } else if(rx_dat != NULL) {
        err = app_spi_read(&vs_spi_dev, &rx_buf_set);
    } else {
        LOG_ERR("No valid TX or RX data provided");
        err = -EINVAL;
    }
    
    if(err) {
        LOG_ERR("SPI transfer failed, err: %d", err);
    }
    
    // Small delay before deasserting CS
    k_usleep(1);
    
    // Deassert chip select AFTER transfer
    if(type == SPI_DATA) {
        gpio_pin_set_dt(&vs_gpio_dcs, 1);  // Deassert DCS
    } else if(type == SPI_CTRL) {
        gpio_pin_set_dt(&vs_gpio_mcs, 1);  // Deassert MCS
    }
}

//< VS1053 Serial Control Interface Write
/*
* @brief
* writes @param data to register at @param addr in VS1053
*/
void VS1053WriteSci(uint8_t addr, uint16_t data) {

    if(debug == true)
    {
        LOG_INF("WriteSci: addr=0x%02X, data=0x%04X", addr, data);
    }
    
    // Wait for DREQ to go high (chip is ready)
    while(!gpio_pin_get_dt(&vs_gpio_dreq)) {
        k_usleep(10);
    }
    
    uint8_t tx_buf[VS1053_XFER_LEN_B] = {
        SCI_WRITE_FLAG, 
        addr, 
        (uint8_t)(data >> 8), 
        (uint8_t)(data & 0xFF)
    };
    
    if(debug == true)
    {
        LOG_INF("TX: %02X %02X %02X %02X", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);
    }

    app_spi_xfer(SPI_CTRL, tx_buf, NULL, VS1053_XFER_LEN_B);
}

//< VS1053 Serial Control Interface Read
/*
* @brief
* reads contents of register at @param addr in VS1053
*/
uint16_t VS1053ReadSci(uint8_t addr) {
    while(!gpio_pin_get_dt(&vs_gpio_dreq)) {
        k_usleep(10);
    }
    
    uint8_t tx_buf[VS1053_XFER_LEN_B] = {SCI_READ_FLAG, addr, 0, 0};
    uint8_t rx_buf[VS1053_XFER_LEN_B] = {0};
    
    app_spi_xfer(SPI_CTRL, tx_buf, rx_buf, VS1053_XFER_LEN_B);
    
    uint16_t res = (rx_buf[2] << 8) | rx_buf[3];
    if(debug == true)
    {
        LOG_INF("ReadSci: addr=0x%02X, RX: %02X %02X %02X %02X, result=0x%04X", 
        addr, rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], res);
    }
    
    return res;
}

//< VS1053 Serial Data Interface Write
/*
* @brief
* writes @param data of @param len direct to memory in VS1053
*/
int VS1053WriteSdi(const uint8_t *data, uint8_t len) {
    if (len > SDI_MAX_PACKET_LEN) {
        LOG_ERR("Data length %d exceeds maximum packet size %d", len, SDI_MAX_PACKET_LEN);
        return -EINVAL;
    }
    
    if (data == NULL) {
        LOG_ERR("Invalid data pointer");
        return -EINVAL;
    }
        
    // Wait for DREQ to go high (chip is ready) with timeout
    while(!gpio_pin_get_dt(&vs_gpio_dreq)) {
        k_sleep(K_USEC(1));
    }
    
    // Use the new app_spi_xfer function for data transfer
    // SPI_DATA type automatically handles DCS pin control
    app_spi_xfer(SPI_DATA, (uint8_t *)data, NULL, len);
    if(debug == true)
    {
        LOG_INF("SDI write successful, %d bytes written", len);
    }

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

    LOG_INF("Loading MIDI Plugin");
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
    LOG_INF("MIDI Plugin Loaded");
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
    LOG_INF("Writing to SCI_MODE");
    VS1053WriteSci(SCI_MODE, SM_SDINEW | SM_SDISHARE | SM_TESTS | SM_RESET);
    VS1053ReadSci(SCI_MODE);
    VS1053WriteSci(SCI_CLOCKF, 0xC000);
    VS1053ReadSci(SCI_CLOCKF);

    LOG_INF("Writing to SCI_AICTRL");
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
    //setup_vs1053_realtime_midi();

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
    
    LOG_INF("Starting VS1053 Initialization");
    
    // Check GPIO readiness
    if (!gpio_is_ready_dt(&vs_gpio_dreq) || !gpio_is_ready_dt(&vs_gpio_reset) ||
        !gpio_is_ready_dt(&vs_gpio_mcs) || !gpio_is_ready_dt(&vs_gpio_dcs)) 
    {
        LOG_ERR("One or more GPIO devices not ready");
        return;
    }
    LOG_INF("All GPIO devices ready");

    // Check SPI readiness
    app_spi_is_ready(&vs_spi_dev);

    // Configure RESET pin (start in reset state - active low means set to 1 for reset)
    ret = gpio_pin_configure_dt(&vs_gpio_reset, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring reset pin: %d", ret);
        return;
    }
    gpio_pin_set_dt(&vs_gpio_reset, 1);  // Put in reset initially

    // Configure DREQ as input
    ret = gpio_pin_configure_dt(&vs_gpio_dreq, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error configuring DREQ pin: %d", ret);
        return;
    }

    // Configure MCS (SCI chip select) - should be inactive (HIGH) initially
    ret = gpio_pin_configure_dt(&vs_gpio_mcs, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring MCS pin: %d", ret);
        return;
    }
    gpio_pin_set_dt(&vs_gpio_mcs, 0);  // 0 = inactive for CS pins

    // Configure DCS (SDI chip select) - should be inactive (HIGH) initially  
    ret = gpio_pin_configure_dt(&vs_gpio_dcs, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring DCS pin: %d", ret);
        return;
    }
    gpio_pin_set_dt(&vs_gpio_dcs, 0);  // 0 = inactive for CS pins
    
    // Release from reset
    gpio_pin_set_dt(&vs_gpio_reset, 0);  // 0 = inactive = not in reset
    
    // Wait for DREQ to indicate ready
    int timeout = 1000; // 1 second timeout
    while(!gpio_pin_get_dt(&vs_gpio_dreq) && timeout > 0) {
        k_msleep(1);
        timeout--;
        if (timeout % 100 == 0) {
            LOG_INF("Still waiting for DREQ... (%d ms left)", timeout);
        }
    }
    
    if (timeout == 0) {
        LOG_ERR("DREQ never went HIGH after reset - check hardware connections!");
        return;
    }
    
    // Now try software initialization
    LOG_INF("Starting VS1053 software reset and initialization...");
    if (VS1053SoftwareReset() != 1) {
        LOG_ERR("Software reset failed!");
        return;
    }
    
    LOG_INF("VS1053 initialization completed successfully!");
}