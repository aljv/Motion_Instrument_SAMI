#ifndef ENCODER_INTERFACE_H
#define ENCODER_INTERFACE_H

//
//NOTE
//      THIS FILE IS ACTUALLY THE STATE_MACHINE FILE, NAME HAS TO BE CHANGED STILL
//      AS RIGHT NOW IT IS encoder_interface.h

#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

// TODO - Patrick: This file is actually going to be the state_machine file. Name needs
//                 to be changed and CMake has to be changed as well.

#define MAX_NUM_BTNS    8
#define NUM_ENCODERS    2

//These are the settings accessed via the right encoder (ENC1)
enum music
{
    INVALID_SETTINGS = -1,
    MUSIC_IDLE = 0,
    SET_TRACK,
    SET_INSTRUMENT,
    SET_TEMPO,
    NUM_SETTINGS,
};

//These are the settings accessed via the left encoder (ENC2)
enum operation
{
    INVALID_MENU = -1,
    OPERATION_IDLE = 0,
    INPUT_MENU,
    PLAYBACK_MENU,
    NUM_OPERATIONS, 
};

//These are the input modes that can be selected by ENC2
enum input_modes 
{
    PLAYMODE_INVALID = -1,
    PLAYMODE_SINGLE_BTN = 0, 
    PLAYMODE_MULTI_BTN,
    PLAYMODE_BLE,
    NUM_PLAYMODES,
};

//These are the playback modes for single button input
enum single_btn_play_modes
{
    PLAYBACK_SINGLE_INVALID = -1,
    PLAYBACK_SINGLE_LATCH = 0,
    PLAYBACK_SINGLE_MOMENTARY,
    PLAYBACK_SINGLE_PLAYPAUSE,
    PLAYBACK_SINGLE_RHYTHM,
    PLAYBACK_SINGLE_NOTE,
    NUM_SINGLE_PLAYBACK, 
};

//These are the playback modes for multi button input
enum multi_btn_play_modes
{
    PLAYBACK_MULTI_INVALID = -1,
    PLAYBACK_MULTI_NOTE = 0, 
    PLAYBACK_MULTI_CHORD,
    PLAYBACK_MULTI_ARP,
    NUM_MULTI_PLAYBACK,
};

// TODO - Undetermined: this enum will hold the various playback modes that will be associated
//                      with the bluetooth input, the individual names can be changed later once
//                      we determine what the bluetooth playmodes will be (this is placeholder)
enum ble_play_modes
{
    PLAYBACK_BLE_INVALID = -1,
    PLAYBACK_BLE_0 = 0,
    PLAYBACK_BLE_1,
    PLAYBACK_BLE_2,
    NUM_BLE_PLAYBACK,
};

//These are the playback states when using single button input
enum playback_states
{
    PLAYBACK_PLAY = 0,
    PLAYBACK_PAUSE,
    PLAYBACK_STOP,
};

//This struct holds all playback modes for all 3 input modes
typedef struct
{
    enum single_btn_play_modes single_btn_play_mode;
    enum multi_btn_play_modes multi_btn_play_mode;
    enum ble_play_modes ble_play_mode;

}play_modes_struct;

//This struct holds the settings for both encoders
typedef struct
{
    enum music music_settings;
    enum operation operation_settings;

}settings_select_struct;

//This is the full state machine struct, holding all current configurations
typedef struct
{
    enum input_modes input_mode;
    enum playback_states playback_state;

    settings_select_struct settings_menu;
    play_modes_struct play_mode;
    enum single_btn_play_modes prev_single_btn_mode;

    uint8_t instrument;
    uint16_t tempo;
    uint8_t octave;
    // TODO - Undetermined: The struct below will be made in a new file, copy from Segger SAMI file called
    //                      "note_generator.h", in this file we will have the enum (music_key_enum)
    //                      with all possible keys (there's 12) and then three functions, 1) get_note
    //                      2) get_chord 3) get_key_name
    //music_key_enum key;

    uint8_t current_track;
    uint8_t previous_track;
    uint8_t total_tracks;
    char track_name[20];

    bool pause;
    bool first_track_entry;
    bool arp_entry;

    bool screen_blackout_entry;
    bool draw_ui_entry;

    // TODO - Patrick: Check if these two are needed, we already have an array "btn_change[MAX_NUM_BTNS]"
    //                 for all the buttons, so i dont think we need seperate ones here
    bool btn7_change; //for polling button
    bool btn8_change; //for polling button

    //Note from Patrick:    I don't think we need pluck since its only used for strum mode which we arent using
    //                      that's why it's commented out right now  
    volatile bool btn[MAX_NUM_BTNS]; //store if button has been pressed or released
    volatile bool encbtn[NUM_ENCODERS]; //store if encoder has been pressed or released
    volatile bool btn_change[MAX_NUM_BTNS]; //if a button state change is detected
    //volatile bool pluck[MAX_NUM_BTNS];

    volatile int notes[MAX_NUM_BTNS];

    uint8_t low_bat_led;

}fsm_struct;

#endif // ENCODER_INTERFACE_H
