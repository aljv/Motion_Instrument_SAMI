#pragma once

#include <zephyr/types.h>
#include <zephyr/kernel.h>

//TODO - SOMEONE: Once this file name is changed this must be replaced
#include "inputs_interface/encoder_interface.h"

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
#define MAX_LINES           2
#define SerLCD_ADDR 0x72

#define SerLCD_SETTING_MODE     0x7C
#define SerLCD_SPECIAL_MODE     0xFE
#define SerLCD_WIDTH_20         0x03
#define SerLCD_WIDTH_16         0x04
#define SerLCD_LINES_4          0x05
#define SerLCD_LINES_2          0x06
#define SerLCD_LINES_1          0x07
#define SerLCD_SOFT_RESET       0x08
#define SerLCD_TOGGLE_SPLASH    0x09
#define SerLCD_SET_SPLASH       0x0A
#define SerLCD_SET_BAUD2400     0x0B
#define SerLCD_SET_BAUD4800     0x0C 
#define SerLCD_SET_BAUD9600     0x0D 
#define SerLCD_SET_BAUD14400    0x0E 
#define SerLCD_SET_BAUD19200    0x0F 
#define SerLCD_SET_BAUD38400    0x10 
#define SerLCD_SET_BAUD57600    0x11 
#define SerLCD_SET_BAUD115200   0x12 
#define SerLCD_SET_BAUD230400   0x13 
#define SerLCD_SET_BAUD460800   0x14 
#define SerLCD_SET_BAUD921600   0x15 
#define SerLCD_SET_BAUD1000000  0x16 
#define SerLCD_SET_BAUD1200     0x17 
#define SerLCD_SET_CONTRAST     0x18
#define SerLCD_SET_TWIADDR      0x19
#define SerLCD_TOGGLE_IGNORERX  0x1A
#define SerLCD_SET_BACKLIGHT    0x2B
#define SerLCD_DISPLAY_VERSION  0x2C
#define SerLCD_DISPLAY_CLEAR    0x2D
#define SerLCD_ENABLE_INFO      0x2E
#define SerLCD_DISABLE_INFO     0x2F
#define SerLCD_ENABLE_SPLASH    0x30
#define SerLCD_DISABLE_SPLASH   0x31
#define SerLCD_SET_PRI_BRIGHT   0x80 /* 0x80-0x9D -> 0-100% */
#define SerLCD_SET_GRE_BRIGHT   0x9E /* 0x9E-0xBB -> 0-100% */
#define SerLCD_SET_BLU_BRIGHT   0xBC /* 0xBC-0xD9 -> 0-100% */  

// special commands
#define SerLCD_RETURNHOME 0x02
#define SerLCD_ENTRYMODESET 0x04
#define SerLCD_DISPLAYCONTROL 0x08
#define SerLCD_CURSORSHIFT 0x10
#define SerLCD_SETDDRAMADDR 0x80

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
void i2c_lcd_clear();