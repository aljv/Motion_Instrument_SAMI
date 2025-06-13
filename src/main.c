/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
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
#include "hw_interface/spi_interface.h"
#include "hw_interface/i2c_interface.h"
#include "hw_interface/inputs_interface/buttons_interface.h"

// TODO - Patrick: IMPORTANT
//                 this include has to be changed to state_machine.h once the file is changed
//    
#include "hw_interface/inputs_interface/encoder_interface.h"

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
#define MODULE main
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

//
//Function declarations
//
void GPIO_Init();
void input_interrupt_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins);
void BTN7_Handler(fsm_struct* fsm);
void BTN8_Handler(fsm_struct* fsm);
void ENC1_Handler(fsm_struct* fsm);
void ENC2_Handler(fsm_struct* fsm);
void UI_Handler(fsm_struct* fsm);
void draw_all_UI(void);
void updateVSvolume(void);
//TODO - Someone: Write these functions somewhere in main
    //void play_track(struct midi_track* track_list, enum playback_states playbackState, uint32_t current_time);
    //void all_playback_notes_off(void);

//
//Global variables
//
//New track selected flag
bool track_new = false;
//Interrupt callback structs
static struct gpio_callback input_cb_gpio0;
static struct gpio_callback input_cb_gpio1;
//Encoder directions
enum encoder_dir enc1_dir;
enum encoder_dir enc2_dir;
//Instruments
#define MAX_INSTRUMENTS 13
#define MIN_INSTRUMENTS 1
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
//Tempo limits
#define MAX_TEMPO 240
#define MIN_TEMPO 40
//ADC & battery charging
#define CHRG_PT   725 //needs charging
#define FULL_CHRG 835 //fully charged
bool charge_bat = false;
int adc = 0;
//MIDI
#define MAX_MIDI_DATA_LENGTH        4092
#define MAX_MIDI_TRACK_LENGTH       4092
#define MAX_MIDI_TRACKS             2
#define DEFAULT_MIDI_NOTE_DURATION  960
uint8_t midi_data[MAX_MIDI_DATA_LENGTH];
//TODO - Someone: Replace with proper structs once midi_parser.h/.c file are written
    //struct midi_track_list[MAX_MIDI_TRACKS];
    //struct notes_being_processed notes_processedd[MAX_MIDI_TRACK_LENGTH];

//Set initial state of fsm struct
fsm_struct fsm = {
    .input_mode = PLAYMODE_SINGLE_BTN,
    .playback_state = PLAYBACK_STOP,

    .settings_menu=
    {
        .music_settings = MUSIC_IDLE,
        .operation_settings = OPERATION_IDLE,
    },

    .play_mode =
    {
        .single_btn_play_mode = PLAYBACK_SINGLE_MOMENTARY,
        .multi_btn_play_mode = PLAYBACK_MULTI_NOTE,
        .ble_play_mode = PLAYBACK_BLE_0,
    },

    .instrument = 0,
    .tempo = 120, //standard tempo
    .octave = 3, //middle C

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

    .notes = {60, 64, 67, 72}, //placeholders

    .low_bat_led = 0,
};

//Function to initialize 100ms app timer for 

//Function to initialize all buttons/encoders and general purpose interrupt function
void GPIO_Init(void)
{
    //If err_code != 0, there was an error at that step
    int err_code = 0;

    err_code = ButtonsInit();
    if (err_code != 0){
        LOG_INF("Failed to initialize buttons\n");
        return;
    }
    err_code = EncodersInit();
    if (err_code != 0){
        LOG_INF("Failed to initialize encoders\n");
        return;
    }

    err_code = PWR_LED_Init();
    if (err_code != 0){
        LOG_INF("Failed to initialize PWR LED\n");
        return;
    }

    //Configure pins as interrupts (encoders and BTN1 to BTN6)
    gpio_pin_interrupt_configure_dt(&ENC1SW, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&ENC2SW, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN1, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN2, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN3, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN4, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN5, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN6, GPIO_INT_EDGE_BOTH);

    //Initialize the gpio0 port callback
    gpio_init_callback(&input_cb_gpio0, input_interrupt_handler,  BIT(ENC1SW.pin)
                                                                | BIT(BTN5.pin)
                                                                | BIT(BTN6.pin));
    //Initialize the gpio1 port callback
    gpio_init_callback(&input_cb_gpio1, input_interrupt_handler,  BIT(ENC2SW.pin)
                                                                | BIT(BTN1.pin)
                                                                | BIT(BTN2.pin)
                                                                | BIT(BTN3.pin)
                                                                | BIT(BTN4.pin));

    //Attach pins to general purpose interrupt handler
    //Only need to do this once for GPIO0 and GPIO1
    gpio_add_callback(ENC1SW.port, &input_cb_gpio0);
    gpio_add_callback(ENC2SW.port, &input_cb_gpio1);

    return;
}

//General purpose interrupt function for BTN1-6 and ENC1/ENC2
void input_interrupt_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    //Encoder 1 triggered interrupt
    if ((pins & BIT(ENC1SW.pin)) && port == ENC1SW.port)
    {
        //If encoder 1 pressed (cycle music settings)
        if (fsm.encbtn[0] == false)
        {
            fsm.encbtn[0] = true;
            
            //Reset song when encoder pressed
            fsm.playback_state = PLAYBACK_STOP;
            fsm.first_track_entry = false;
            fsm.pause = false;
            
            //So long as other encoder (2) isn't being pressed
            if (fsm.settings_menu.operation_settings == OPERATION_IDLE)
            {
                fsm.settings_menu.music_settings++;
                //Handle setting wrapping
                if(fsm.settings_menu.music_settings == NUM_SETTINGS)
                {
                    fsm.settings_menu.music_settings = MUSIC_IDLE;
                }
            }

            if (fsm.settings_menu.music_settings != MUSIC_IDLE && fsm.settings_menu.operation_settings == OPERATION_IDLE)
            {
                LOG_INF("ENC1 - Screen blackout\n");
                fsm.screen_blackout_entry = true;
                fsm.draw_ui_entry = false;
            }
            else if (fsm.settings_menu.music_settings == MUSIC_IDLE && fsm.settings_menu.operation_settings == OPERATION_IDLE)
            {
                LOG_INF("ENC1 - Draw all UI\n");
                fsm.screen_blackout_entry = false;
                fsm.draw_ui_entry = true;
            }

            LOG_INF("SET1\n");

        }
        //Else, encoder 1 released
        else
        {
            fsm.encbtn[0] = false;
            LOG_INF("EXIT1\n");
        }
    }

    //Encoder 2 triggered interrupt
    else if ((pins & BIT(ENC2SW.pin)) && port == ENC2SW.port)
    {
        //If encoder 2 pressed (cycle operation settings)
        if (fsm.encbtn[1] == false)
        {
            fsm.encbtn[1] = true;

            //Reset song when encoder pressed
            fsm.playback_state = PLAYBACK_STOP;
            fsm.first_track_entry = false;
            fsm.pause = false;

            //So long as other encoder(1) isn't being pressed
            if (fsm.settings_menu.music_settings == MUSIC_IDLE)
            {
                fsm.settings_menu.operation_settings++;
                //Handle wrapping
                if (fsm.settings_menu.operation_settings == NUM_OPERATIONS)
                {
                    fsm.settings_menu.operation_settings = OPERATION_IDLE;
                }
            }

            if (fsm.settings_menu.operation_settings != OPERATION_IDLE && fsm.settings_menu.music_settings == MUSIC_IDLE)
            {
                LOG_INF("ENC2 - Screen blackout\n");
                fsm.screen_blackout_entry = true;
                fsm.draw_ui_entry = false;
            }
            else if (fsm.settings_menu.operation_settings == OPERATION_IDLE && fsm.settings_menu.music_settings == MUSIC_IDLE)
            {
                LOG_INF("ENC2 - Draw all UI\n");
                fsm.screen_blackout_entry = false;
                fsm.draw_ui_entry = true;
            }

            LOG_INF("SET2\n");

        }
        else
        {
            fsm.encbtn[1] = false;
            LOG_INF("EXIT2\n");
        }
    }

    //Button 1 triggered interrupt
    else if ((pins & BIT(BTN1.pin)) && port == BTN1.port)
    {
        //Set btn changed flag to true
        fsm.btn_change[0] = true;
        if (fsm.btn[0] == false)
        {
            fsm.btn[0] = true;
            LOG_INF("BTN1 = 1\n");
        }
        else
        {
            fsm.btn[0] = false;
            LOG_INF("BTN1 = 0\n");
        }
    }

    //Button 2 triggered interrupt
    else if ((pins & BIT(BTN2.pin)) && port == BTN2.port)
    {
        //Set btn changed flag to true
        fsm.btn_change[1] = true;
        if (fsm.btn[1] == false)
        {
            fsm.btn[1] = true;
            LOG_INF("BTN2 = 1\n");
        }
        else
        {
            fsm.btn[1] = false;
            LOG_INF("BTN2 = 0\n");
        }
    }

    //Button 3 triggered interrupt
    else if ((pins & BIT(BTN3.pin)) && port == BTN3.port)
    {
        //Set btn changed flag to true
        fsm.btn_change[2] = true;
        if (fsm.btn[2] == false)
        {
            fsm.btn[2] = true;
            LOG_INF("BTN3 = 1\n");
        }
        else
        {
            fsm.btn[2] = false;
            LOG_INF("BTN3 = 0\n");
        }
    }

    //Button 4 triggered interrupt
    else if ((pins & BIT(BTN4.pin)) && port == BTN4.port)
    {
        //Set btn changed flag to true
        fsm.btn_change[3] = true;
        if (fsm.btn[3] == false)
        {
            fsm.btn[3] = true;
            LOG_INF("BTN4 = 1\n");
        }
        else
        {
            fsm.btn[3] = false;
            LOG_INF("BTN4 = 0\n");
        }
    }

    //Button 5 triggered interrupt
    else if ((pins & BIT(BTN5.pin)) && port == BTN5.port)
    {
        //Set btn changed flag to true
        fsm.btn_change[4] = true;
        if (fsm.btn[4] == false)
        {
            fsm.btn[4] = true;
            LOG_INF("BTN5 = 1\n");
        }
        else
        {
            fsm.btn[4] = false;
            LOG_INF("BTN5 = 0\n");
        }
    }
    
    //Button 6 triggered interrupt
    else if ((pins & BIT(BTN6.pin)) && port == BTN6.port)
    {
        //Set btn changed flag to true
        fsm.btn_change[5] = true;
        if (fsm.btn[5] == false)
        {
            fsm.btn[5] = true;
            LOG_INF("BTN6 = 1\n");
        }
        else
        {
            fsm.btn[5] = false;
            LOG_INF("BTN6 = 0\n");
        }
    }
}

//Function to poll the state of BTN7
//TODO - PATRICK: Confirm that button is active low,
//                if not then need to take out "!" in first if
//                and put it into the second if
void BTN7_Handler(fsm_struct* fsm)
{
    if (!gpio_pin_get_dt(&BTN7) && fsm->btn[6] == false && fsm->btn_change[6] == false)
    {
        fsm->btn_change[6] = true;
        fsm->btn[6] = true;
        LOG_INF("BTN7 = 1\n");
    }
    else if (gpio_pin_get_dt(&BTN7) && fsm->btn[6] == true && fsm->btn_change[6] == true)
    {
        fsm->btn_change[6] = false;
        fsm->btn[6] = false;
        LOG_INF("BTN7 = 0\n");
    }
}

//Function to poll the state of BTN8
//TODO - PATRICK: Confirm that button is active low,
//                if not then need to take out "!" in first if
//                and put it into the second if
void BTN8_Handler(fsm_struct* fsm)
{
    if (!gpio_pin_get_dt(&BTN8) && fsm->btn[7] == false && fsm->btn_change[7] == false)
    {
        fsm->btn_change[7] = true;
        fsm->btn[7] = true;
        LOG_INF("BTN8 = 1\n");
    }
    else if (gpio_pin_get_dt(&BTN8) && fsm->btn[7] == true && fsm->btn_change[7] == true)
    {
        fsm->btn_change[7] = false;
        fsm->btn[7] = false;
        LOG_INF("BTN8 = 0\n");
    }
}

//Function to handle track/instrument/tempo selection
void ENC1_Handler(fsm_struct* fsm)
{
    if (fsm->settings_menu.music_settings != MUSIC_IDLE && fsm->settings_menu.operation_settings == OPERATION_IDLE)
    {
        //arp_stop();  //Pat note: copied from old SAMI, don't know what this is
        switch(fsm->settings_menu.music_settings)
        {
            case SET_TRACK:
                //TODO - Alex: Replace this with lcd blackout and draw functions - not written yet
                //i2c_lcd_clear();
                //i2c_lcd_draw_track(fsm->current_track);

                //Stay changing track until encoder1 pressed again
                while (fsm->settings_menu.music_settings == SET_TRACK)
                {
                    enc1_dir = get_enc1_dir();
                    fsm->previous_track = fsm->current_track;
                    switch (enc1_dir)
                    {
                        case ENC_CW:
                            fsm->current_track++;
                            if (fsm->current_track > fsm->total_tracks)
                            {
                                fsm->current_track = 1;
                            }
                            else if (fsm->current_track == 0)
                            {
                                fsm->current_track = fsm->total_tracks;
                            }

                            LOG_INF("Track CW\n");
                            LOG_INF("%d_Track.mid selected\n", fsm->current_track);
                            //TODO - Alex: Replace this with lcd write function - not written yet
                            //i2c_lcd_draw_track(fsm->current_track);

                            //If new song is selected, update key and tempo
                            if (fsm->current_track != fsm->previous_track)
                            {
                                track_new = true;
                                //TODO - Alex: Replace with correct functions once they are written
                                //
                                //fsm->tempo = MICROSECONDS_PER_MIN / track_list[0].tempo;
                                //arpTempo_set(fsm->tempo); //Copied from old SAMI, dont know again
                            }

                            break;

                        case ENC_CCW:
                            fsm->current_track--;
                            if (fsm->current_track > fsm->total_tracks)
                            {
                                fsm->current_track = 1;
                            }
                            else if (fsm->current_track == 0)
                            {
                                fsm->current_track = fsm->total_tracks;
                            }

                            LOG_INF("Track CW\n");
                            LOG_INF("%d_Track.mid selected\n", fsm->current_track);
                            //TODO - Alex: Replace this with lcd write functions - not written yet
                            //i2c_lcd_draw_track(fsm->current_track);

                            //If new song is selected, update key and tempo
                            if (fsm->current_track != fsm->previous_track)
                            {
                                //Set track_new flag to be handled in main
                                track_new = true;
                                //TODO - Alex: Replace with correct functions once they are written
                                //fsm->tempo = MICROSECONDS_PER_MIN / track_list[0].tempo;
                                //arpTempo_set(fsm->tempo); //Copied from old SAMI, dont know again
                            }

                            break;
                    }
                }

                fsm->first_track_entry = false;
                break;

            case SET_INSTRUMENT:
                //TODO - Alex: Replace this with lcd blackout function and lcd write function - not written yet
                //i2c_lcd_clear();
                //i2c_lcd_draw_instrument(fsm->instrument);

                //Stay changing instrument until encoder1 pressed again
                while(fsm->settings_menu.music_settings == SET_INSTRUMENT)
                {
                    enc1_dir = get_enc1_dir();
                    switch (enc1_dir)
                    {
                        case ENC_CW:
                            fsm->instrument++;
                            if (fsm->instrument > MAX_INSTRUMENTS)
                            {
                                fsm->instrument = MIN_INSTRUMENTS;
                            }
                            else if (fsm->instrument < MIN_INSTRUMENTS)
                            {
                                fsm->instrument = MAX_INSTRUMENTS;
                            }
                            LOG_INF("Instrument CW\n");
                            //TODO - Alex: Replace this with lcd blackout function and lcd write function - not written yet
                            //i2c_lcd_draw_instrument(fsm->instrument);
                            break;
                        
                        case ENC_CCW:
                            fsm->instrument--;
                            if (fsm->instrument > MAX_INSTRUMENTS)
                            {
                                fsm->instrument = MIN_INSTRUMENTS;
                            }
                            else if (fsm->instrument < MIN_INSTRUMENTS)
                            {
                                fsm->instrument = MAX_INSTRUMENTS;
                            }
                            LOG_INF("Instrument CCW\n");
                            //TODO - Alex: Replace this with lcd blackout function and lcd write function - not written yet
                            //i2c_lcd_draw_instrument(fsm->instrument);
                            break;
                    }
                }
                //TODO - SOMEONE: Replace this with function that sets instrument in midi file - not created yet
                //midi_set_instrument(0, VS1053_Instrument[fsm->instrument]);
                break;

            case SET_TEMPO:
                //TODO - Alex: Replace this with lcd blackout function and lcd write functions - not written yet
                //i2c_lcd_clear();
                //i2c_lcd_draw_tempo(fsm->tempo);

                //Stay changing tempo until encoder1 pressed again
                while (fsm->settings_menu.music_settings == SET_TEMPO)
                {
                    enc1_dir = get_enc1_dir();
                    switch (enc1_dir)
                    {
                        case ENC_CW:
                            fsm->tempo += 1;
                            if (fsm->tempo > MAX_TEMPO)
                            {
                                fsm->tempo = MAX_TEMPO;
                            }
                            else if (fsm->tempo < MIN_TEMPO)
                            {
                                fsm->tempo = MIN_TEMPO;
                            }

                            LOG_INF("Tempo CW\n");
                            
                            //TODO - Alex: Replace this with lcd write function - not written yet
                            //i2c_lcd_draw_tempo(fsm->tempo);
                            break;
                        
                        case ENC_CCW:
                            fsm->tempo -= 1;
                            if (fsm->tempo > MAX_TEMPO)
                            {
                                fsm->tempo = MAX_TEMPO;
                            }
                            else if (fsm->tempo < MIN_TEMPO)
                            {
                                fsm->tempo = MIN_TEMPO;
                            }

                            LOG_INF("Tempo CW\n");
                            
                            //TODO - Alex: Replace this with lcd write function - not written yet
                            //i2c_lcd_draw_tempo(fsm->tempo);
                            break;
                    }
                }
                //arpTempo_set(fsm->tempo); //Copied from old SAMI, Not sure what this is for
                break;
                
            default:
                break;
        }
    }
}

//Function to handle input/playback settings selection
void ENC2_Handler(fsm_struct* fsm)
{
    //enum input_modes prev_input_mode = fsm->input_mode; //Pat note: pretty sure we don't need this

    if(fsm->settings_menu.music_settings == MUSIC_IDLE && fsm->settings_menu.operation_settings != OPERATION_IDLE)
    {
        switch (fsm->settings_menu.operation_settings)
        {
            case INPUT_MENU:
                //TODO - Alex: Replace this with lcd blackout and draw functions - not written yet
                //i2c_lcd_clear();
                //i2c_lcd_draw_input(fsm->input_mode);
                //arp_stop();   //Pat note: again don't know what this is, copied from old SAMI, don't think we need it

                while (fsm->settings_menu.operation_settings == INPUT_MENU)
                {
                    enc2_dir = get_enc2_dir();
                    switch (enc2_dir)
                    {
                        case ENC_CW:
                            fsm->input_mode++;
                            if (fsm->input_mode == NUM_PLAYMODES)
                            {
                                fsm->input_mode = PLAYMODE_SINGLE_BTN;
                            }
                            else if (fsm->input_mode == PLAYMODE_INVALID)
                            {
                                fsm->input_mode = PLAYMODE_BLE;
                            }
                            //Set track_new flag to be handled in main
                            track_new = true;
                            LOG_INF("Input CW\n");

                            //TODO - Alex: Replace this with lcd blackout and draw functions - not written yet
                            //i2c_lcd_draw_input(fsm->input_mode);
                            break;
                        
                        case ENC_CCW:
                            fsm->input_mode--;
                            if (fsm->input_mode == NUM_PLAYMODES)
                            {
                                fsm->input_mode = PLAYMODE_SINGLE_BTN;
                            }
                            else if (fsm->input_mode == PLAYMODE_INVALID)
                            {
                                fsm->input_mode = PLAYMODE_BLE;
                            }
                            //Set track_new flag to be handled in main
                            track_new = true;
                            LOG_INF("Input CCW\n");

                            //TODO - Alex: Replace this with lcd draw function - not written yet
                            //i2c_lcd_draw_input(fsm->input_mode);
                            break;
                    }
                }

                break;
            
            case PLAYBACK_MENU:
                //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                //i2c_lcd_clear();
                //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);
                //arp_stop();   //Pat note: again don't know what this is, copied from old SAMI, don't think we need it

                switch (fsm->input_mode)
                {
                    case PLAYMODE_SINGLE_BTN:
                        while (fsm->settings_menu.operation_settings == PLAYBACK_MENU)
                        {
                            enc2_dir = get_enc2_dir();
                            switch (enc2_dir)
                            {
                                case ENC_CW:
                                    fsm->play_mode.single_btn_play_mode++;
                                    if (fsm->play_mode.single_btn_play_mode == NUM_SINGLE_PLAYBACK)
                                    {
                                        fsm->play_mode.single_btn_play_mode = PLAYBACK_SINGLE_LATCH;
                                    }
                                    else if (fsm->play_mode.single_btn_play_mode == PLAYBACK_SINGLE_INVALID)
                                    {
                                        fsm->play_mode.single_btn_play_mode = PLAYBACK_SINGLE_NOTE;
                                    }

                                    LOG_INF("Single Btn CW\n");
                                    //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                                    //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

                                    break;
                                
                                case ENC_CCW:
                                    fsm->play_mode.single_btn_play_mode--;
                                    if (fsm->play_mode.single_btn_play_mode == NUM_SINGLE_PLAYBACK)
                                    {
                                        fsm->play_mode.single_btn_play_mode = PLAYBACK_SINGLE_LATCH;
                                    }
                                    else if (fsm->play_mode.single_btn_play_mode == PLAYBACK_SINGLE_INVALID)
                                    {
                                        fsm->play_mode.single_btn_play_mode = PLAYBACK_SINGLE_NOTE;
                                    }

                                    LOG_INF("Single Btn CCW\n");
                                    //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                                    //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

                                    break; 
                            }
                        }
                        break;

                    case PLAYMODE_MULTI_BTN:
                        while (fsm->settings_menu.operation_settings == PLAYBACK_MENU)
                        {
                            enc2_dir = get_enc2_dir();
                            switch (enc2_dir)
                            {
                                case ENC_CW:
                                    fsm->play_mode.multi_btn_play_mode++;
                                    if (fsm->play_mode.multi_btn_play_mode == NUM_MULTI_PLAYBACK)
                                    {
                                        fsm->play_mode.multi_btn_play_mode = PLAYBACK_MULTI_NOTE;
                                    }
                                    else if (fsm->play_mode.multi_btn_play_mode == PLAYBACK_MULTI_INVALID)
                                    {
                                        fsm->play_mode.multi_btn_play_mode = PLAYBACK_MULTI_ARP;
                                    }

                                    LOG_INF("Multi Btn CW\n");
                                    //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                                    //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

                                    break;
                                
                                case ENC_CCW:
                                    fsm->play_mode.multi_btn_play_mode--;
                                    if (fsm->play_mode.multi_btn_play_mode == NUM_MULTI_PLAYBACK)
                                    {
                                        fsm->play_mode.multi_btn_play_mode = PLAYBACK_MULTI_NOTE;
                                    }
                                    else if (fsm->play_mode.multi_btn_play_mode == PLAYBACK_MULTI_INVALID)
                                    {
                                        fsm->play_mode.multi_btn_play_mode = PLAYBACK_MULTI_ARP;
                                    }

                                    LOG_INF("Multi Btn CCW\n");
                                    //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                                    //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

                                    break; 
                            }
                        }
                        break;

                    case PLAYMODE_BLE:
                        while (fsm->settings_menu.operation_settings == PLAYBACK_MENU)
                        {
                            enc2_dir = get_enc2_dir();
                            switch (enc2_dir)
                            {
                                case ENC_CW:
                                    fsm->play_mode.ble_play_mode++;
                                    if (fsm->play_mode.ble_play_mode == NUM_BLE_PLAYBACK)
                                    {
                                        fsm->play_mode.ble_play_mode = PLAYBACK_BLE_0;
                                    }
                                    else if (fsm->play_mode.ble_play_mode == PLAYBACK_BLE_INVALID)
                                    {
                                        fsm->play_mode.ble_play_mode = PLAYBACK_BLE_2;
                                    }

                                    LOG_INF("BLE CW\n");
                                    //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                                    //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);
                                    break;
                                
                                case ENC_CCW:
                                    fsm->play_mode.ble_play_mode--;
                                    if (fsm->play_mode.ble_play_mode == NUM_BLE_PLAYBACK)
                                    {
                                        fsm->play_mode.ble_play_mode = PLAYBACK_BLE_0;
                                    }
                                    else if (fsm->play_mode.ble_play_mode == PLAYBACK_BLE_INVALID)
                                    {
                                        fsm->play_mode.ble_play_mode = PLAYBACK_BLE_2;
                                    }

                                    LOG_INF("BLE CCW\n");
                                    //TODO - Alex: Replace this with lcd blackout and write functions - not written yet
                                    //i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);
                                    break; 
                            }
                        }
                        break;
                    
                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    /* Pat note - pretty sure we don't need this
    if (prev_input_mode != fsm->input_mode)
    {
        midiSetInstrument(etc.)
    }
    */
}

//Function to handle LCD clearing and drawing when encoders are pressed
void UI_Handler(fsm_struct* fsm)
{
    if (fsm->screen_blackout_entry)
    {
        //TODO - Alex: Replace this with lcd clear function - not written yet
        //i2c_lcd_clear();
        fsm->screen_blackout_entry = false;
    }
    else if (fsm->draw_ui_entry)
    {
        //TODO - Patrick: Replace this with function to write settings to SD - not written yet
        //write_to_sd(fsm);
        draw_all_UI();
        fsm->draw_ui_entry = false;
    }
}

//Function to draw all LCD information
void draw_all_UI()
{
    //TODO - ALEX: Replace all commented out function with LCD writing functions
    //             none written yet

    //Delay 50microseconds, k_busy_wait should not interrupt bluetooth code functionality
    k_busy_wait(50);
    //i2c_lcd_draw_input(fsm.input_mode);

    k_busy_wait(50);
    //i2c_lcd_draw_playback(fsm.input_mode, fsm.play_mode);

    k_busy_wait(50);
    //i2c_lcd_draw_song(fsm.current_track);

    k_busy_wait(50);
    //i2c_lcd_draw_key(fsm.key);

    k_busy_wait(50);
    //i2c_lcd_draw_instrument(fsm.instrument);

    k_busy_wait(50);
    //i2c_lcd_draw_tempo(fsm.tempo);

    k_busy_wait(50);
}

//
//Main function
//
int main(void)
{
    LOG_INF("Start of main\n");
    GPIO_Init();

    //TESTING PURPOSES - This works (PWR LED is red)
    LOG_INF("Setting PWR LED");
    gpio_pin_set_dt(&PowerLED, 1);

    //AppTimer_Init();
    //AppTimer_Start();
    //Saadc_Init();

    CheckDevices();
    SDcardInterfaceInit(); //Error here when attempting to run - see sd_card_interface.c
    dk_leds_init();
    dk_set_led_on(DK_LED1);
    LOG_INF("LED turned on\n");

    while (1)
    {
        //dk_set_led_off(DK_LED1);
        //k_sleep(K_SECONDS(1));

        BTN7_Handler(&fsm);
        BTN8_Handler(&fsm);

        UI_Handler(&fsm);

        ENC1_Handler(&fsm);
        ENC2_Handler(&fsm);

        //If track_new is set, we need to parse the new song and get the new key/tempo
        if(track_new)
        {
            //TODO - Alex: Replace all below with correct functions once they are written
            //
            //midi_parse_file(fsm.track_name, track_list, midi_data, notes_processedd);
            //fsm.key = infer_key(track_list);
            //i2c_lcd_drawKey(fsm.key);
            //fsm.tempo = MICROSECONDS_PER_MINUTE / track_list[0].tempo;
            //arpTempo_set(fsm.tempo);
            track_new = false;
        }

        k_sleep(K_SECONDS(1));

    }

    return 0;
}

