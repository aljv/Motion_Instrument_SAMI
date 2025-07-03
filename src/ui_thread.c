/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
// ui_thread.c - Updated to use the new GPIO interface
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/hrs_client.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>

#include "hw_interface/ble_interface/ble_interface.h"
#include "hw_interface/sd_card_interface/sd_card_interface.h"
#include "hw_interface/VS1053_interface/VS1053_interface.h"
#include "hw_interface/VS1053_interface/midi.h"
#include "hw_interface/spi_interface.h"
#include "hw_interface/i2c_interface.h"
#include "hw_interface/uart_interface.h"
#include "hw_interface/inputs_interface/gpio_interface.h"  // Use the new GPIO interface

#include "state_machine_defs.h"

#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#define MODULE ui_thread
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

uint8_t VS1053_Instrument[] = {     0, //OFFSET
                                    1, //ACOUSTIC_GRAND_PIANO
                                    2, //ELECTRIC_GRAND_PIANO
                                    3, //ACOUSTIC_GUITAR
                                    4, //ELECTRIC_GUITAR
                                    5, //XYLOPHONE
                                    6, //ELECTRIC_BASS
                                    7, //BAG_PIPE
                                    8, //WOOD_BLOCK
                                    9, //ALTO_SAXAPHONE
                                    10, //TRUMPET
                                    11, //FLUTE
                                    12, //TROMBONE
                                    13, //VIOLIN
                                };

// FSM structure (same as your original)
fsm_struct fsm = {
    .input_mode = PLAYMODE_SINGLE_BTN,
    .playback_state = PLAYBACK_STOP,
    .settings_menu = {
        .music_settings = MUSIC_IDLE,
        .operation_settings = OPERATION_IDLE,
    },
    .play_mode = {
        .single_btn_play_mode = PLAYBACK_SINGLE_MOMENTARY,
        .multi_btn_play_mode = PLAYBACK_MULTI_NOTE,
        .ble_play_mode = PLAYBACK_BLE_0,
    },
    .instrument = 0,
    .tempo = 120,
    .octave = 3,
    .current_track = 1,
    .previous_track = 1,
    .track_name = "1_TRACK.mid",
    .pause = false,
    .first_track_entry = false,
    .arp_entry = false,
    .screen_blackout_entry = false,
    .draw_ui_entry = false,
    .btn = {false, false, false, false, false, false, false, false},
    .encbtn = {false, false},
    .btn_change = {false, false, false, false, false, false, false, false},
    .notes = {60, 64, 67, 72},
    .low_bat_led = 0,
};

// Function to draw all LCD information (same as original)
void draw_all_UI(void)
{
    k_busy_wait(50);
    i2c_lcd_draw_input(fsm.input_mode);

    k_busy_wait(50);
    i2c_lcd_draw_playback(fsm.input_mode, fsm.play_mode);

    k_busy_wait(50);
    i2c_lcd_draw_track(fsm.current_track);

    k_busy_wait(50);
    i2c_lcd_draw_instrument(fsm.instrument);

    k_busy_wait(50);
    i2c_lcd_draw_tempo(fsm.tempo);

    k_busy_wait(50);
}

// Updated interrupt handler using the new GPIO interface
void input_interrupt_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    // Get GPIO specs using the interface functions
    const struct gpio_dt_spec* enc1sw = get_enc1sw_gpio();
    const struct gpio_dt_spec* enc2sw = get_enc2sw_gpio();
    const struct gpio_dt_spec* btn1 = get_btn1_gpio();
    const struct gpio_dt_spec* btn2 = get_btn2_gpio();
    const struct gpio_dt_spec* btn3 = get_btn3_gpio();
    const struct gpio_dt_spec* btn4 = get_btn4_gpio();
    const struct gpio_dt_spec* btn5 = get_btn5_gpio();
    const struct gpio_dt_spec* btn6 = get_btn6_gpio();

    // Encoder 1 triggered interrupt
    if ((pins & BIT(enc1sw->pin)) && port == enc1sw->port) {
        if (fsm.encbtn[0] == false) {
            fsm.encbtn[0] = true;
            
            // Reset song when encoder pressed
            fsm.playback_state = PLAYBACK_STOP;
            fsm.first_track_entry = false;
            fsm.pause = false;
            
            // So long as other encoder (2) isn't being pressed
            if (fsm.settings_menu.operation_settings == OPERATION_IDLE) {
                fsm.settings_menu.music_settings++;
                // Handle setting wrapping
                if(fsm.settings_menu.music_settings == NUM_SETTINGS) {
                    fsm.settings_menu.music_settings = MUSIC_IDLE;
                }
            }

            if (fsm.settings_menu.music_settings != MUSIC_IDLE && fsm.settings_menu.operation_settings == OPERATION_IDLE) {
                LOG_INF("ENC1 - Screen blackout");
                fsm.screen_blackout_entry = true;
                fsm.draw_ui_entry = false;
            } else if (fsm.settings_menu.music_settings == MUSIC_IDLE && fsm.settings_menu.operation_settings == OPERATION_IDLE) {
                LOG_INF("ENC1 - Draw all UI");
                fsm.screen_blackout_entry = false;
                fsm.draw_ui_entry = true;
            }

            LOG_INF("SET1");
        } else {
            fsm.encbtn[0] = false;
            LOG_INF("EXIT1");
        }
    }

    // Encoder 2 triggered interrupt
    else if ((pins & BIT(enc2sw->pin)) && port == enc2sw->port) {
        if (fsm.encbtn[1] == false) {
            fsm.encbtn[1] = true;

            // Reset song when encoder pressed
            fsm.playback_state = PLAYBACK_STOP;
            fsm.first_track_entry = false;
            fsm.pause = false;

            // So long as other encoder(1) isn't being pressed
            if (fsm.settings_menu.music_settings == MUSIC_IDLE) {
                fsm.settings_menu.operation_settings++;
                // Handle wrapping
                if (fsm.settings_menu.operation_settings == NUM_OPERATIONS) {
                    fsm.settings_menu.operation_settings = OPERATION_IDLE;
                }
            }

            if (fsm.settings_menu.operation_settings != OPERATION_IDLE && fsm.settings_menu.music_settings == MUSIC_IDLE) {
                LOG_INF("ENC2 - Screen blackout");
                fsm.screen_blackout_entry = true;
                fsm.draw_ui_entry = false;
            } else if (fsm.settings_menu.operation_settings == OPERATION_IDLE && fsm.settings_menu.music_settings == MUSIC_IDLE) {
                LOG_INF("ENC2 - Draw all UI");
                fsm.screen_blackout_entry = false;
                fsm.draw_ui_entry = true;
            }

            LOG_INF("SET2");
        } else {
            fsm.encbtn[1] = false;
            LOG_INF("EXIT2");
        }
    }

    // Button interrupts - using the new interface
    else if ((pins & BIT(btn1->pin)) && port == btn1->port) {
        fsm.btn_change[0] = true;
        fsm.btn[0] = !fsm.btn[0];
        LOG_INF("BTN1 = %d", fsm.btn[0]);
    }
    else if ((pins & BIT(btn2->pin)) && port == btn2->port) {
        fsm.btn_change[1] = true;
        fsm.btn[1] = !fsm.btn[1];
        LOG_INF("BTN2 = %d", fsm.btn[1]);
    }
    else if ((pins & BIT(btn3->pin)) && port == btn3->port) {
        fsm.btn_change[2] = true;
        fsm.btn[2] = !fsm.btn[2];
        LOG_INF("BTN3 = %d", fsm.btn[2]);
    }
    else if ((pins & BIT(btn4->pin)) && port == btn4->port) {
        fsm.btn_change[3] = true;
        fsm.btn[3] = !fsm.btn[3];
        LOG_INF("BTN4 = %d", fsm.btn[3]);
    }
    else if ((pins & BIT(btn5->pin)) && port == btn5->port) {
        fsm.btn_change[4] = true;
        fsm.btn[4] = !fsm.btn[4];
        LOG_INF("BTN5 = %d", fsm.btn[4]);
    }
    else if ((pins & BIT(btn6->pin)) && port == btn6->port) {
        fsm.btn_change[5] = true;
        fsm.btn[5] = !fsm.btn[5];
        LOG_INF("BTN6 = %d", fsm.btn[5]);
    }
}

// Function to handle LCD clearing and drawing when encoders are pressed
void UI_Handler(fsm_struct* fsm)
{
    if (fsm->screen_blackout_entry) {
        i2c_lcd_clear();
        fsm->screen_blackout_entry = false;
    } else if (fsm->draw_ui_entry) {
        // TODO - Patrick: Replace this with function to write settings to SD - not written yet
        // write_to_sd(fsm);
        draw_all_UI();
        fsm->draw_ui_entry = false;
    }
}

// Updated GPIO initialization function using the new interface
int GPIO_Init_UI_Thread(void)
{
    int err_code = 0;
    
    LOG_INF("Initializing GPIO interface for UI thread");
    
    // Initialize the GPIO interface
    err_code = GPIO_Init();
    if (err_code != 0) {
        LOG_ERR("Failed to initialize GPIO interface: %d", err_code);
        return err_code;
    }
    
    // The interrupt callbacks are already set up in GPIO_Init(),
    // but we can override the handler if needed
    
    LOG_INF("GPIO interface initialized successfully for UI thread");
    return 0;
}
