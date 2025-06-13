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

// SPI callback function
void spi_callback(const struct device *dev, int result, void *data)
{
    spi_xfer_done = true;
}

// TEST CODE - DELETE AFTER
// Test patterns for register testing
static const uint16_t test_patterns[] = {
    0x0000, 0xFFFF, 0x5555, 0xAAAA, 
    0x1234, 0xABCD, 0x00FF, 0xFF00,
    0x0F0F, 0xF0F0, 0x3333, 0xCCCC
};

// Register test structure
typedef struct {
    uint8_t addr;
    const char* name;
    bool read_only;
    bool writable;
    uint16_t reset_value;
    uint16_t write_mask;  // Bits that can be written (0xFFFF = all bits writable)
    const char* description;
} vs1053_register_t;

// VS1053 Register map with properties
static const vs1053_register_t vs1053_registers[] = {
    {SCI_MODE, "SCI_MODE", false, true, 0x4800, 0xFFFF, "Mode control register"},
    {SCI_STATUS, "SCI_STATUS", false, true, 0x000C, 0xFFFF, "Status register"},
    {SCI_BASS, "SCI_BASS", false, true, 0x0000, 0xFFFF, "Bass/treble control"},
    {SCI_CLOCKF, "SCI_CLOCKF", false, true, 0x0000, 0xFFFF, "Clock frequency control"},
    {SCI_DECODE_TIME, "SCI_DECODE_TIME", false, true, 0x0000, 0xFFFF, "Decode time in seconds"},
    {SCI_AUDATA, "SCI_AUDATA", false, true, 0x0000, 0xFFFF, "Audio data (samplerate/channels)"},
    {SCI_WRAM, "SCI_WRAM", false, true, 0x0000, 0xFFFF, "RAM read/write"},
    {SCI_WRAMADDR, "SCI_WRAMADDR", false, true, 0x0000, 0xFFFF, "RAM address"},
    {SCI_HDAT0, "SCI_HDAT0", true, false, 0x0000, 0x0000, "Stream header data 0 (read-only)"},
    {SCI_HDAT1, "SCI_HDAT1", true, false, 0x0000, 0x0000, "Stream header data 1 (read-only)"},
    {SCI_AIADDR, "SCI_AIADDR", false, true, 0x0000, 0xFFFF, "Application start address"},
    {SCI_VOL, "SCI_VOL", false, true, 0x0000, 0xFFFF, "Volume control"},
    {SCI_AICTRL0, "SCI_AICTRL0", false, true, 0x0000, 0xFFFF, "Application control 0"},
    {SCI_AICTRL1, "SCI_AICTRL1", false, true, 0x0000, 0xFFFF, "Application control 1"},
    {SCI_AICTRL2, "SCI_AICTRL2", false, true, 0x0000, 0xFFFF, "Application control 2"},
    {SCI_AICTRL3, "SCI_AICTRL3", false, true, 0x0000, 0xFFFF, "Application control 3"}
};

#define NUM_REGISTERS (sizeof(vs1053_registers) / sizeof(vs1053_registers[0]))
#define NUM_TEST_PATTERNS (sizeof(test_patterns) / sizeof(test_patterns[0]))

// Basic register read test
bool test_register_read(uint8_t addr, const char* name) {
    LOG_INF("Testing READ %s (0x%02X)", name, addr);
    
    uint16_t value = VS1053ReadSci(addr);
    
    // Check for obviously invalid values (all 0s or all 1s might indicate problems)
    if (value == 0x0000) {
        LOG_WRN("  %s read as 0x0000 (might be OK, or could indicate problem)", name);
    } else if (value == 0xFFFF) {
        LOG_WRN("  %s read as 0xFFFF (might indicate SPI problem)", name);
    } else {
        LOG_INF("  %s = 0x%04X ✓", name, value);
    }
    
    return true;
}

// Basic register write test
bool test_register_write(uint8_t addr, const char* name, uint16_t test_value, uint16_t write_mask) {
    LOG_INF("Testing WRITE %s (0x%02X) with value 0x%04X", name, addr, test_value);
    
    // Read original value
    uint16_t original = VS1053ReadSci(addr);
    LOG_INF("  Original value: 0x%04X", original);
    
    // Write test value
    VS1053WriteSci(addr, test_value);
    k_msleep(10);  // Small delay for register update
    
    // Read back
    uint16_t readback = VS1053ReadSci(addr);
    LOG_INF("  Wrote: 0x%04X, Read back: 0x%04X", test_value, readback);
    
    // Check if write was successful (considering write mask)
    uint16_t expected = test_value & write_mask;
    uint16_t actual = readback & write_mask;
    
    bool success = (expected == actual);
    
    if (success) {
        LOG_INF("  Write test PASSED ✓");
    } else {
        LOG_ERR("  Write test FAILED ✗ (Expected: 0x%04X, Got: 0x%04X)", expected, actual);
    }
    
    // Restore original value
    VS1053WriteSci(addr, original);
    k_msleep(10);
    
    return success;
}

// Comprehensive register test for a single register
bool test_single_register_comprehensive(const vs1053_register_t* reg) {
    LOG_INF("=== Comprehensive test for %s ===", reg->name);
    
    bool all_passed = true;
    
    // Test 1: Basic read
    if (!test_register_read(reg->addr, reg->name)) {
        all_passed = false;
    }
    
    // Test 2: Write tests (only for writable registers)
    if (reg->writable) {
        LOG_INF("Testing write patterns for %s", reg->name);
        
        for (int i = 0; i < NUM_TEST_PATTERNS; i++) {
            if (!test_register_write(reg->addr, reg->name, test_patterns[i], reg->write_mask)) {
                all_passed = false;
            }
            k_msleep(50);  // Delay between tests
        }
        
        // Test 3: Bit walking test
        LOG_INF("Bit walking test for %s", reg->name);
        
        for (int bit = 0; bit < 16; bit++) {
            uint16_t test_val = (1 << bit);
            if (!test_register_write(reg->addr, reg->name, test_val, reg->write_mask)) {
                all_passed = false;
            }
            k_msleep(10);
        }
        
        // Test 4: Inverted bit walking test
        LOG_INF("Inverted bit walking test for %s", reg->name);
        
        for (int bit = 0; bit < 16; bit++) {
            uint16_t test_val = ~(1 << bit);
            if (!test_register_write(reg->addr, reg->name, test_val, reg->write_mask)) {
                all_passed = false;
            }
            k_msleep(10);
        }
        
    } else {
        LOG_INF("%s is read-only, skipping write tests", reg->name);
    }
    
    return all_passed;
}

// Test all VS1053 registers
void test_all_registers_basic(void) {
    LOG_INF("=== VS1053 Basic Register Test ===");
    
    int passed = 0;
    int total = 0;
    
    for (int i = 0; i < NUM_REGISTERS; i++) {
        const vs1053_register_t* reg = &vs1053_registers[i];
        
        LOG_INF("\n--- Testing %s (0x%02X) ---", reg->name, reg->addr);
        LOG_INF("Description: %s", reg->description);
        
        bool result = test_register_read(reg->addr, reg->name);
        
        if (result) passed++;
        total++;
        
        k_msleep(100);  // Delay between register tests
    }
    
    LOG_INF("\n=== Basic Register Test Results ===");
    LOG_INF("Passed: %d/%d registers", passed, total);
    
    if (passed == total) {
        LOG_INF("All basic register reads PASSED ✓");
    } else {
        LOG_ERR("Some register reads FAILED ✗");
    }
}

// Test specific writable registers with write/read cycles
void test_writable_registers(void) {
    LOG_INF("=== VS1053 Writable Register Test ===");
    
    int passed = 0;
    int total = 0;
    
    for (int i = 0; i < NUM_REGISTERS; i++) {
        const vs1053_register_t* reg = &vs1053_registers[i];
        
        if (!reg->writable) {
            LOG_INF("Skipping read-only register %s", reg->name);
            continue;
        }
        
        LOG_INF("\n--- Testing writable register %s ---", reg->name);
        
        // Test with a simple pattern first
        bool result = test_register_write(reg->addr, reg->name, 0x5555, reg->write_mask);
        
        if (result) passed++;
        total++;
        
        k_msleep(100);
    }
    
    LOG_INF("\n=== Writable Register Test Results ===");
    LOG_INF("Passed: %d/%d writable registers", passed, total);
    
    if (passed == total) {
        LOG_INF("All writable register tests PASSED ✓");
    } else {
        LOG_ERR("Some writable register tests FAILED ✗");
    }
}

// Quick communication test
void test_vs1053_communication(void) {
    LOG_INF("=== VS1053 Communication Test ===");
    
    // Test 1: Read STATUS register (should never be 0x0000 or 0xFFFF)
    uint16_t status = VS1053ReadSci(SCI_STATUS);
    LOG_INF("STATUS register: 0x%04X", status);
    
    if (status == 0x0000) {
        LOG_ERR("STATUS = 0x0000 - possible SPI communication failure");
        return;
    } else if (status == 0xFFFF) {
        LOG_ERR("STATUS = 0xFFFF - possible SPI communication failure");
        return;
    }
    
    // Test 2: Check chip version
    uint16_t version = (status >> 4) & 0xF;
    LOG_INF("Chip version: %d", version);
    
    if (version == 4) {
        LOG_INF("VS1053 detected ✓");
    } else {
        LOG_WRN("Unexpected chip version: %d (expected 4 for VS1053)", version);
    }
    
    // Test 3: Test a safe writable register (AICTRL1)
    LOG_INF("Testing AICTRL1 register write/read...");
    
    uint16_t original = VS1053ReadSci(SCI_AICTRL1);
    VS1053WriteSci(SCI_AICTRL1, 0x1234);
    k_msleep(10);
    uint16_t readback = VS1053ReadSci(SCI_AICTRL1);
    VS1053WriteSci(SCI_AICTRL1, original);  // Restore
    
    LOG_INF("Wrote 0x1234, read back 0x%04X", readback);
    
    if (readback == 0x1234) {
        LOG_INF("Register write/read test PASSED ✓");
        LOG_INF("VS1053 SCI communication is working correctly!");
    } else {
        LOG_ERR("Register write/read test FAILED ✗");
        LOG_ERR("SCI communication may have issues");
    }
}

// Stress test - rapid register access
void test_vs1053_stress(void) {
    LOG_INF("=== VS1053 Stress Test ===");
    
    const int NUM_ITERATIONS = 100;
    int read_failures = 0;
    int write_failures = 0;
    
    LOG_INF("Performing %d rapid read/write cycles...", NUM_ITERATIONS);
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Test rapid reads
        uint16_t status1 = VS1053ReadSci(SCI_STATUS);
        uint16_t status2 = VS1053ReadSci(SCI_STATUS);
        
        if (status1 != status2) {
            read_failures++;
            LOG_WRN("Read inconsistency at iteration %d: 0x%04X vs 0x%04X", i, status1, status2);
        }
        
        // Test rapid write/read
        uint16_t test_val = (uint16_t)(0x1000 + i);
        VS1053WriteSci(SCI_AICTRL1, test_val);
        uint16_t readback = VS1053ReadSci(SCI_AICTRL1);
        
        if (readback != test_val) {
            write_failures++;
            LOG_WRN("Write/read failure at iteration %d: wrote 0x%04X, read 0x%04X", i, test_val, readback);
        }
        
        if (i % 20 == 0) {
            LOG_INF("Completed %d/%d iterations", i, NUM_ITERATIONS);
        }
    }
    
    // Clean up
    VS1053WriteSci(SCI_AICTRL1, 0x0000);
    
    LOG_INF("=== Stress Test Results ===");
    LOG_INF("Read failures: %d/%d", read_failures, NUM_ITERATIONS);
    LOG_INF("Write failures: %d/%d", write_failures, NUM_ITERATIONS);
    
    if (read_failures == 0 && write_failures == 0) {
        LOG_INF("Stress test PASSED ✓ - SCI communication is stable");
    } else {
        LOG_ERR("Stress test FAILED ✗ - SCI communication has issues");
    }
}

// Main register test function to call from your code
void vs1053_register_test_suite(void) {
    LOG_INF("=== VS1053 Register Test Suite ===");
    
    // Make sure VS1053 is ready
    if (!gpio_pin_get_dt(&vs_gpio_dreq)) {
        LOG_ERR("DREQ is LOW - VS1053 not ready for testing");
        return;
    }
    
    // Run tests in order of increasing complexity
    test_vs1053_communication();
    k_msleep(500);
    
    test_all_registers_basic();
    k_msleep(500);
    
    test_writable_registers();
    k_msleep(500);
    
    test_vs1053_stress();
    
    LOG_INF("=== Register Test Suite Complete ===");
}

// Minimal quick test for debugging
void vs1053_quick_test(void) {
    LOG_INF("=== VS1053 Quick Test ===");
    
    // Just test basic communication
    test_vs1053_communication();
    
    LOG_INF("Quick test complete");
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

void debug_device_tree(void) {
    LOG_INF("=== Device Tree Debug ===");
    LOG_INF("MCS GPIO port: %p, pin: %d, flags: 0x%08X", 
            vs_gpio_mcs.port, vs_gpio_mcs.pin, vs_gpio_mcs.dt_flags);
    LOG_INF("DCS GPIO port: %p, pin: %d, flags: 0x%08X", 
            vs_gpio_dcs.port, vs_gpio_dcs.pin, vs_gpio_dcs.dt_flags);
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
        return;
    }
    
    // Small delay to ensure CS setup time
    k_usleep(1);
    
    // Perform SPI transfer
    if(tx_dat != NULL && rx_dat != NULL) {
        err = spi_transceive_dt(&vs_spi_dev, &tx_buf_set, &rx_buf_set);
    } else if(tx_dat != NULL) {
        err = spi_write_dt(&vs_spi_dev, &tx_buf_set);
    } else if(rx_dat != NULL) {
        err = spi_read_dt(&vs_spi_dev, &rx_buf_set);
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
    LOG_INF("WriteSci: addr=0x%02X, data=0x%04X", addr, data);
    
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
    
    LOG_INF("TX: %02X %02X %02X %02X", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);
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
    LOG_INF("ReadSci: addr=0x%02X, RX: %02X %02X %02X %02X, result=0x%04X", 
            addr, rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], res);
    
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

    LOG_DBG("SDI write successful, %d bytes written", len);
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
    k_msleep(50);
    
    // Release reset pin
    gpio_pin_set_dt(&vs_gpio_reset, 1);
    k_msleep(50);

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
    
    // Configure GPIO pins with proper error checking
    LOG_INF("Configuring GPIO pins...");

    // Configure RESET pin (start in reset state - active low means set to 1 for reset)
    ret = gpio_pin_configure_dt(&vs_gpio_reset, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring reset pin: %d", ret);
        return;
    }
    gpio_pin_set_dt(&vs_gpio_reset, 1);  // Put in reset initially
    LOG_INF("RESET pin configured and set to ACTIVE (reset state)");

    // Configure DREQ as input
    ret = gpio_pin_configure_dt(&vs_gpio_dreq, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error configuring DREQ pin: %d", ret);
        return;
    }
    LOG_INF("DREQ pin configured as input");

    // Configure MCS (SCI chip select) - should be inactive (HIGH) initially
    ret = gpio_pin_configure_dt(&vs_gpio_mcs, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring MCS pin: %d", ret);
        return;
    }
    gpio_pin_set_dt(&vs_gpio_mcs, 0);  // 0 = inactive for CS pins
    LOG_INF("MCS pin configured and set to INACTIVE");

    // Configure DCS (SDI chip select) - should be inactive (HIGH) initially  
    ret = gpio_pin_configure_dt(&vs_gpio_dcs, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring DCS pin: %d", ret);
        return;
    }
    gpio_pin_set_dt(&vs_gpio_dcs, 0);  // 0 = inactive for CS pins
    LOG_INF("DCS pin configured and set to INACTIVE");

    debug_pin_states("After GPIO configuration");
    
    // Release from reset
    LOG_INF("Releasing VS1053 from reset...");
    gpio_pin_set_dt(&vs_gpio_reset, 0);  // 0 = inactive = not in reset
    
    // Wait for DREQ to indicate ready
    LOG_INF("Waiting for DREQ to go high...");
    int timeout = 1000; // 1 second timeout
    while(!gpio_pin_get_dt(&vs_gpio_dreq) && timeout > 0) {
        k_msleep(1);
        timeout--;
        if (timeout % 100 == 0) {
            LOG_INF("Still waiting for DREQ... (%d ms left)", timeout);
            //debug_pin_states("Waiting for DREQ");
        }
    }
    
    if (timeout == 0) {
        LOG_ERR("DREQ never went HIGH after reset - check hardware connections!");
        //debug_pin_states("DREQ timeout");
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


// Alternative version that matches your original pattern more closely:
/*uint16_t VS1053ReadSci_Debug(uint8_t addr) {
    // Wait for DREQ with better timeout handling
    while(gpio_pin_get_dt(&vs_gpio_dreq)) {
        k_sleep(K_USEC(1));
    }
    
    uint8_t tx_buf[4] = {SCI_READ_FLAG, addr};
    uint8_t rx_buf[4] = {0};
    
    // Prepare SPI buffers manually
    struct spi_buf tx_spi_buf = {.buf = tx_buf, .len = 4};
    struct spi_buf rx_spi_buf = {.buf = rx_buf, .len = 4};
    
    // Direct SPI call
    app_spi_xfer(SPI_CTRL, NULL, &rx_buf, VS1053_XFER_LEN_B);
    
    // Debug output
    LOG_INF("RX: [0x%02X, 0x%02X, 0x%02X, 0x%02X]", 
            rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
    
    uint16_t result = (rx_buf[2] << 8) | rx_buf[3];
    LOG_INF("Combined result: 0x%04X", result);
    
    return result;
}


// Test function to verify SPI communication
void VS1053TestSPI(void) {
    LOG_INF("=== VS1053 SPI Communication Test ===");
    VS1053HardwareReset();
    
    // Test 1: Check DREQ pin state
    int dreq_state = gpio_pin_get_dt(&vs_gpio_dreq);
    LOG_INF("DREQ pin state: %d", dreq_state);
    
    // Test 2: Try to read STATUS register
    LOG_INF("Reading STATUS register (0x01)...");
    uint16_t status = VS1053ReadSci_Debug(SCI_STATUS);
    LOG_INF("STATUS: 0x%04X", status);
    
    // Test 3: Try to read MODE register  
    LOG_INF("Reading MODE register (0x00)...");
    uint16_t mode = VS1053ReadSci_Debug(SCI_MODE);
    LOG_INF("MODE: 0x%04X", mode);
    
    // Test 4: Read version
    LOG_INF("Reading version info...");
    uint16_t version = ((status >> 4) & 15);
    LOG_INF("Chip version: %d", version);
}*/
