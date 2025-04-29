#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "i2c_interface.h"

#define MODULE i2c_interface
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define LCD_I2C_NODE DT_NODELABEL(lcd)

static struct i2c_dt_spec dev_lcd_i2c = I2C_DT_SPEC_GET(LCD_I2C_NODE);

// TODOS 
// Add all LCD code from SAMI NRF SDK app.

void i2c_interface_init(void)
{
        if(!device_is_ready(dev_lcd_i2c.bus))
        {
                LOG_ERR("I2C bus %s is not ready\n\r", dev_lcd_i2c.bus->name);
                return;
        }
        else
        {
                LOG_INF("I2C bus initialized");
        }
}

void i2c_lcd_transmit(uint8_t buf)
{
        uint8_t config[] = {buf};
        int ret = i2c_write_dt(&dev_lcd_i2c, config, sizeof(config));

        if(ret != 0)
        {
                LOG_ERR("Failed to write to I2C device address %x at reg. %x \n\r", dev_lcd_i2c.addr, config[0]);
                 return;
        }
        else
        {
                LOG_INF("Successfully wrote to I2C device");
        }

        return;
}

void i2c_lcd_read(uint8_t buf)
{
        int ret = i2c_read_dt(&dev_lcd_i2c, &buf, sizeof(buf));
        
        if(ret != 0)
        {
                LOG_ERR("Failed to read from I2C device address %x\n\r", dev_lcd_i2c.addr);
        }
        else
        {
                LOG_INF("Successfully read from I2C device");
        }

        return;
}

void ser_lcd_write_string(unsigned char *str, size_t len)
{
        size_t str_len = strlen((char*)str);

        // transmit string
        while(*str)
        {
                i2c_lcd_transmit(*str++);
        }

        if(str_len < len)
        {
                size_t space_to_fill = len - str_len;

                for(size_t i = 0; i < space_to_fill; i++)
                {
                        i2c_lcd_transmit(' ');
                }
        }
}

void i2c_lcd_set_cursor(uint8_t c, uint8_t l)
{
        uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
        l = MIN(l, (uint8_t)(MAX_LINES - 1));
        i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
        i2c_lcd_transmit(SerLCD_SETDDRAMADDR | (c + row_offsets[l]));
}

void ser_lcd_init(void)
{
    uint8_t _displayControl = SerLCD_DISPLAYON | SerLCD_CURSOROFF | SerLCD_BLINKOFF;
    uint8_t _displayMode = SerLCD_ENTRYLEFT | SerLCD_ENTRYSHIFTDECREMENT;

    i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
    i2c_lcd_transmit(SerLCD_TOGGLE_SPLASH);
    i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
    i2c_lcd_transmit(SerLCD_DISPLAY_CLEAR);
    
    i2c_lcd_transmit(SerLCD_SETTING_MODE);
    i2c_lcd_transmit(SerLCD_DISABLE_SPLASH);
    i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
    i2c_lcd_transmit(SerLCD_DISPLAY_CLEAR);
    i2c_lcd_transmit(SerLCD_SOFT_RESET);    
    i2c_lcd_transmit(SerLCD_DISPLAY_CLEAR);
    
    i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
    i2c_lcd_transmit(SerLCD_DISPLAYCONTROL | _displayControl);
    
    i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
    i2c_lcd_transmit(SerLCD_ENTRYMODESET | _displayMode); 
    i2c_lcd_transmit(SerLCD_SETTING_MODE);   
    i2c_lcd_transmit(SerLCD_DISPLAY_CLEAR);
    i2c_lcd_transmit(SerLCD_SOFT_RESET);

    i2c_lcd_transmit(SerLCD_SETTING_MODE);   
    i2c_lcd_transmit(SerLCD_DISPLAY_CLEAR);
}