#ifndef I2C_INTERFACE_H
#define I2C_INTERFACE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>

//TODO - SOMEONE: Once this file name is changed this must be replaced
#include "state_machine_defs.h"

#define INPUT_MODE_DISPLAY_AREA_LENGTH    10
#define INSTRUMENT_DISPLAY_AREA_LENGTH  2
#define TRACK_DISPLAY_AREA_LENGTH       2
#define TEMPO_DISPLAY_AREA_LENGTH       3
#define KEY_DISPLAY_AREA_LENGTH         2

#define LCD_INSTRUMENT_ROW_CURSOR  1  
#define LCD_INSTRUMENT_COL_CURSOR 10 
#define LCD_TEMPO_COL_CURSOR      13
#define LCD_TEMPO_ROW_CURSOR      1
#define LCD_TRACK_COL_CURSOR      10
#define LCD_TRACK_ROW_CURSOR      0
#define LCD_KEY_COL_CURSOR        13
#define LCD_KEY_ROW_CURSOR        0
#define LCD_MENU_COL_CURSOR       0
#define LCD_MENU_ROW_CURSOR       0
#define LCD_MODE_COL_CURSOR       0
#define LCD_MODE_ROW_CURSOR       1


/*** SerLCD ***/
//<
#define SerLCD_DISPLAY_ADDRESS1 0x72 //This is the default address of the OpenLCD
#define SerLCD_DISPLAY_ADDRESS2 0x73 //This is the default address of the OpenLCD

//OpenLCD command characters
#define SerLCD_SPECIAL_MODE 0xFE //Magic number for special commands
#define SerLCD_SETTING_MODE 0x7C //Magic number for setting commands

//Special commands
#define SerLCD_DISPLAY_CLEAR 0x01
#define SerLCD_DISPLAY_RETURN_HOME 0x02
#define SerLCD_DISPLAY_MOVE_CURSOR_LEFT 0x10
#define SerLCD_DISPLAY_MOVE_CURSOR_RIGHT 0x14
#define SerLCD_DISPLAY_TURN_ON 0x0C
#define SerLCD_DISPLAY_TURN_OFF 0x08
#define SerLCD_UNDERLINE_CURSOR_ON 0x0E
#define SerLCD_UNDERLINE_CURSOR_OFF 0x0C
#define SerLCD_BLINKING_CURSOR_ON 0x0F
#define SerLCD_BLINKING_CURSOR_OFF 0x0E
#define SerLCD_SET_CURSOR 0x80
#define SerLCD_SCROLL_RIGHT 0x1C
#define SerLCD_SCROLL_LEFT 0x18
#define SerLCD_MOVE_CURSOR_RIGHT 0x14
#define SerLCD_MOVE_CURSOR_LEFT 0x10

//Setting commands
#define SerLCD_TOGGLE_SPLASH 0x09 //0x09 will turn on the splash screen
#define SerLCD_SAVE_CURRENT_DISPLAY_AS_SPLASH 0x0A //0x0A will save whatever is currently being displayed into EEPROM
//This will cause a 2.2 second delay
#define SerLCD_CHANGE_ADDRESS 0x19 //0x19 will change the I2C address
#define SerLCD_SET_RGB 0x2B //0x2B will set the red, green, and blue values of the backlight
#define SerLCD_DISABLE_SPLASH 0x00
#define SerLCD_ENABLE_SPLASH 0x01

// rgb backlight update
#define SerLCD_SET_PRI_BRIGHT   0x80 /* 0x80-0x9D -> 0-100% */
#define SerLCD_SET_GRE_BRIGHT   0x9E /* 0x9E-0xBB -> 0-100% */
#define SerLCD_SET_BLU_BRIGHT   0xBC /* 0xBC-0xD9 -> 0-100% */

//Enable system messages to be displayed to LCD
#define SerLCD_ENABLE_SYSTEM_MESSAGE 0x03
#define SerLCD_DISABLE_SYSTEM_MESSAGE 0x04

//Enable/disable ignore RX pin on OpenLCD
#define SerLCD_ENABLE_IGNORE_RX 0x05
#define SerLCD_DISABLE_IGNORE_RX 0x06

//Enable different firmware 
#define SerLCD_SOFT_RESET 0x08

#define SerLCD_SETDDRAMADDR 0x80

// special commands
#define SerLCD_ENTRYLEFT 0x02
#define SerLCD_ENTRYSHIFTDECREMENT 0x00

// flags for display entry mode
#define SerLCD_ENTRYRIGHT 0x00
#define SerLCD_ENTRYLEFT 0x02
#define SerLCD_ENTRYSHIFTINCREMENT 0x01
#define SerLCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define SerLCD_DISPLAYON 0x04
#define SerLCD_DISPLAYOFF 0x00
#define SerLCD_CURSORON 0x02
#define SerLCD_CURSOROFF 0x00
#define SerLCD_BLINKON 0x01
#define SerLCD_BLINKOFF 0x00

// flags for display/cursor shift
#define SerLCD_DISPLAYMOVE 0x08
#define SerLCD_CURSORMOVE 0x00
#define SerLCD_MOVERIGHT 0x04
#define SerLCD_MOVELEFT 0x00

#define SerLCD_DISPLAYCONTROL 0x08
#define SerLCD_ENTRYMODESET 0x04

#define MAX_LINES 4

/*** Audio Amplifier MAX9744 ***/
#define AUDIO_AMP_ADDR 0x49        // Audio amplifier I2C address (ADDR2=low, ADDR1=high)
#define MAXIMUM_AMP_VOL 63         // +9.5dB, see MAX9744 datasheet
#define DEFAULT_AMP_VOL 45         // -0.5dB
#define MIN_AMP_VOL 0              // Minimum volume (-50dB)

// Function prototypes
void i2c_interface_init(void);
void i2c_lcd_transmit(uint8_t buf);
void i2c_lcd_read(uint8_t buf);
void i2c_lcd_set_cursor(uint8_t c, uint8_t l);

void ser_lcd_write_string(unsigned char *str, size_t len);
void ser_lcd_write_int(int n);
void ser_lcd_init(void);

void i2c_lcd_draw_input(enum input_modes input_mode);
void i2c_lcd_draw_playback(enum input_modes input_mode, play_modes_struct play_mode);
void i2c_lcd_draw_track(uint8_t track);
//TODO - Uncomment this function once midi file is written
//void i2c_lcd_draw_key(music_key_enum key);
void i2c_lcd_draw_instrument(uint8_t instr);
void i2c_lcd_draw_tempo(uint16_t tempo);
void i2c_lcd_clear(void);

// Audio amplifier functions
void max9744_init(void);
void max9744_set_volume(uint8_t volume);
uint8_t max9744_get_volume(void);
void max9744_mute(void);
void max9744_unmute(void);

int audio_amplifier_gpio_init(void);
void audio_amplifier_hardware_enable(void);
void audio_amplifier_hardware_disable(void);

#endif // I2C_INTERFACE_H