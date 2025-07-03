#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

#include "i2c_interface.h"
#include "VS1053_interface/VS1053_interface.h"
#include "state_machine_defs.h"

#define MODULE i2c_interface
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define LCD_I2C_NODE DT_NODELABEL(lcd)
#define AMP_I2C_NODE DT_NODELABEL(audio_amp)

static struct i2c_dt_spec dev_lcd_i2c = I2C_DT_SPEC_GET(LCD_I2C_NODE);
static struct i2c_dt_spec dev_amp_i2c = I2C_DT_SPEC_GET(AMP_I2C_NODE);

// GPIO definitions for audio amplifier control (if needed)
#define AMP_MUTE_NODE DT_ALIAS(ampmute)
#define AMP_MAX_MUTE_NODE DT_ALIAS(ampmaxmute)
#define AMP_MAX_SHDN_NODE DT_ALIAS(ampmaxshdn)

static const struct gpio_dt_spec amp_mute = GPIO_DT_SPEC_GET(DT_NODELABEL(amp_mute), gpios);
static const struct gpio_dt_spec amp_max_mute = GPIO_DT_SPEC_GET(DT_NODELABEL(amp_max_mute), gpios);
static const struct gpio_dt_spec amp_max_shdn = GPIO_DT_SPEC_GET(DT_NODELABEL(amp_max_shdn), gpios);

static uint8_t current_amp_volume = DEFAULT_AMP_VOL - 5;

//Copy of fsm_struct to hold things that are being written to LCD
fsm_struct fsm_copy;

// TODOS 
// Add all LCD code from SAMI NRF SDK app.

void i2c_interface_init(void)
{
        if(!device_is_ready(dev_lcd_i2c.bus))
        {
                LOG_ERR("I2C LCD bus %s is not ready\n\r", dev_lcd_i2c.bus->name);
                return;
        }
        else
        {
                LOG_INF("I2C LCD bus initialized");
        }

        if(!device_is_ready(dev_amp_i2c.bus))
        {
                LOG_ERR("I2C AMP bus %s is not ready\n\r", dev_lcd_i2c.bus->name);
                return;
        }
        else
        {
                LOG_INF("I2C AMP bus initialized");
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
                LOG_DBG("Successfully wrote to I2C LCD device");
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
                LOG_DBG("Successfully read from I2C LCD device");
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

void ser_lcd_write_int(int n)
{
        char n_s[4];
        snprintf(n_s, sizeof(n_s), "%d", n);
        if (n < 10)
        {
                i2c_lcd_transmit(n_s[0]);
        }
        else if (n >= 10 && n < 100)
        {
                i2c_lcd_transmit(n_s[0]);
                i2c_lcd_transmit(n_s[1]);
        }
        else if (n > 99)
        {
                i2c_lcd_transmit(n_s[0]);
                i2c_lcd_transmit(n_s[1]);
                i2c_lcd_transmit(n_s[2]);
        }
        else if (n > 999)
        {
                LOG_INF("lcd_write_int: int too big");
        }
        else
        {
                i2c_lcd_transmit(' ');
        }
}

void i2c_lcd_set_cursor(uint8_t c, uint8_t l)
{
        uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
        l = MIN(l, (uint8_t)(MAX_LINES - 1));
        i2c_lcd_transmit(SerLCD_SPECIAL_MODE);
        i2c_lcd_transmit(SerLCD_SETDDRAMADDR | (c + row_offsets[l]));
}

void i2c_lcd_draw_input(enum input_modes input_mode)
{
        fsm_copy.input_mode = input_mode;
        i2c_lcd_set_cursor(0,0);
        switch (input_mode)
        {
                case PLAYMODE_SINGLE_BTN:
                        ser_lcd_write_string("SNGL BTN", INPUT_MODE_DISPLAY_AREA_LENGTH);
                        break;
                case PLAYMODE_MULTI_BTN:
                        ser_lcd_write_string("MLTI BTN", INPUT_MODE_DISPLAY_AREA_LENGTH);
                        break;
                case PLAYMODE_BLE:
                        ser_lcd_write_string("BT MODE", INPUT_MODE_DISPLAY_AREA_LENGTH);
                        break;
                default:
                        break;

        }
}

void i2c_lcd_draw_playback(enum input_modes input_mode, play_modes_struct play_mode)
{
        fsm_copy.input_mode = input_mode;
        fsm_copy.play_mode = play_mode;
        i2c_lcd_set_cursor(0,1);
        switch(input_mode)
        {
                case PLAYMODE_SINGLE_BTN:
                        switch (play_mode.single_btn_play_mode)
                        {
                                case PLAYBACK_SINGLE_LATCH:
                                        ser_lcd_write_string("LATCH", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_SINGLE_MOMENTARY:
                                        ser_lcd_write_string("MOMENTARY", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_SINGLE_PLAYPAUSE:
                                        ser_lcd_write_string("PLAY/PAUS", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_SINGLE_NOTE:
                                        ser_lcd_write_string("SNGL NOTE", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                        }
                        break;
                case PLAYMODE_MULTI_BTN:
                        switch (play_mode.multi_btn_play_mode)
                        {
                                case PLAYBACK_MULTI_NOTE:
                                        ser_lcd_write_string("NOTE", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_MULTI_CHORD:
                                        ser_lcd_write_string("CHORD", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_MULTI_ARP:
                                        ser_lcd_write_string("ARP", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                default:
                                        break;
                        }
                        break;
                case PLAYMODE_BLE:
                        switch (play_mode.ble_play_mode)
                        {
                                case PLAYBACK_BLE_0:
                                        ser_lcd_write_string("BT 1", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_BLE_1:
                                        ser_lcd_write_string("BT 2", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                case PLAYBACK_BLE_2:
                                        ser_lcd_write_string("BT 3", INPUT_MODE_DISPLAY_AREA_LENGTH);
                                        break;
                                default:
                                        break;
                        }
                        break;
        }
}

void i2c_lcd_draw_track(uint8_t track)
{
        fsm_copy.current_track = track;
        i2c_lcd_set_cursor(LCD_TRACK_COL_CURSOR+1, LCD_TRACK_ROW_CURSOR);
        ser_lcd_write_string(" ", 1);
        i2c_lcd_set_cursor(LCD_TRACK_COL_CURSOR, LCD_TRACK_ROW_CURSOR);
        ser_lcd_write_int(track);
}

/*TODO - Uncomment this function once midi file is written
void i2c_lcd_draw_key(music_key_enum key)
{

}
*/

void i2c_lcd_draw_instrument(uint8_t instr)
{
        fsm_copy.instrument = instr;
        static uint8_t prev_instr = 0;

        if (prev_instr >= 10 && instr < 10)
        {
                i2c_lcd_set_cursor(LCD_INSTRUMENT_COL_CURSOR+1, LCD_INSTRUMENT_ROW_CURSOR);
                ser_lcd_write_string(" ", 1);
        }

        i2c_lcd_set_cursor(LCD_INSTRUMENT_COL_CURSOR, LCD_INSTRUMENT_ROW_CURSOR);
        ser_lcd_write_int(instr);

        prev_instr = instr;

}

void i2c_lcd_draw_tempo(uint16_t tempo)
{
        fsm_copy.tempo = tempo;
        i2c_lcd_set_cursor(LCD_TEMPO_COL_CURSOR, LCD_TEMPO_ROW_CURSOR);

        if (tempo < 99)
        {
                ser_lcd_write_int(tempo);
                i2c_lcd_set_cursor(LCD_TEMPO_COL_CURSOR+2, LCD_TEMPO_ROW_CURSOR);
                ser_lcd_write_string(" ", 1);
        }
        else
        {
                ser_lcd_write_int(tempo);
        }
}

void i2c_lcd_clear(void)
{
        i2c_lcd_set_cursor(0,0);
        ser_lcd_write_string(" ", 16);
        i2c_lcd_set_cursor(0,1);
        ser_lcd_write_string(" ", 16);
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

/*** Audio Amplifier MAX9744 Functions ***/

void max9744_init(void)
{
        // Initialize the audio amplifier with default volume
        LOG_INF("Initializing MAX9744 audio amplifier");
        max9744_set_volume(DEFAULT_AMP_VOL);
}

void max9744_set_volume(uint8_t volume)
{
        // Clamp volume to valid range
        if(volume > MAXIMUM_AMP_VOL)
        {
                volume = MAXIMUM_AMP_VOL;
                LOG_WRN("Volume clamped to maximum: %d", MAXIMUM_AMP_VOL);
        }

        uint8_t config[] = {volume};
        int ret = i2c_write_dt(&dev_amp_i2c, config, sizeof(config));


        if(ret != 0)
        {
                LOG_ERR("Failed to write to audio amplifier at address 0x%02x (error: %d)", 
                        AUDIO_AMP_ADDR, ret);
                return;
        }
        else
        {
                current_amp_volume = volume;
                LOG_INF("Audio amplifier volume set to: %d", volume);
        }
}

uint8_t max9744_get_volume(void)
{
        return current_amp_volume;
}

void max9744_mute(void)
{
        LOG_INF("Muting audio amplifier");
        max9744_set_volume(MIN_AMP_VOL);
}

void max9744_unmute(void)
{
        LOG_INF("Unmuting audio amplifier");
        // Restore to default volume if currently muted, otherwise keep current volume
        if(current_amp_volume == MIN_AMP_VOL)
        {
                max9744_set_volume(DEFAULT_AMP_VOL);
        }
}

int audio_amplifier_gpio_init(void)
{
        int ret;

        
        // Initialize MAX_SHDN pin (active low for shutdown, so start with high to enable)
        if (!gpio_is_ready_dt(&amp_max_shdn)) {
                LOG_ERR("Audio amplifier MAX_SHDN GPIO not ready");
                return -ENODEV;
        }
        
        ret = gpio_pin_configure_dt(&amp_max_shdn, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
                LOG_ERR("Cannot configure audio amplifier MAX_SHDN GPIO (%d)", ret);
                return ret;
        }

        // Initialize MUTE pin
        if (!gpio_is_ready_dt(&amp_mute)) {
                LOG_ERR("Audio amplifier MUTE GPIO not ready");
                return -ENODEV;
        }
        
        ret = gpio_pin_configure_dt(&amp_mute, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
                LOG_ERR("Cannot configure audio amplifier MUTE GPIO (%d)", ret);
                return ret;
        }

        // Initialize MAX_MUTE pin
        if (!gpio_is_ready_dt(&amp_max_mute)) {
                LOG_ERR("Audio amplifier MAX_MUTE GPIO not ready");
                return -ENODEV;
        }
        
        ret = gpio_pin_configure_dt(&amp_max_mute, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
                LOG_ERR("Cannot configure audio amplifier MAX_MUTE GPIO (%d)", ret);
                return ret;
        }

        audio_amplifier_hardware_disable();
        
        LOG_INF("Audio amplifier GPIO pins initialized");

        return 0;
}

void audio_amplifier_hardware_enable(void)
{
        // Ensure amplifier is not in shutdown mode
        gpio_pin_set_dt(&amp_max_shdn, 1);
        
        // Ensure amplifier is not muted via GPIO
        gpio_pin_set_dt(&amp_mute, 1);
        gpio_pin_set_dt(&amp_max_mute, 1);
        
        LOG_INF("Audio amplifier hardware enabled");
}

void audio_amplifier_hardware_disable(void)
{
        // Mute the amplifier
        gpio_pin_set_dt(&amp_max_mute, 0);
        gpio_pin_set_dt(&amp_mute, 0);
        
        // Put amplifier in shutdown mode
        //gpio_pin_set_dt(&amp_max_shdn, 0);
        
        LOG_INF("Audio amplifier hardware disabled");
}

void get_amp_gpio_pin_sates(void)
{
        LOG_INF("Mute %d", gpio_pin_get_dt(&amp_mute));
        LOG_INF("Max Mute %d", gpio_pin_get_dt(&amp_max_mute));
        LOG_INF("SHDN %d", gpio_pin_get_dt(&amp_max_shdn));
}
