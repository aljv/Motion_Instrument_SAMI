// gpio_interface.c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "gpio_interface.h"

LOG_MODULE_REGISTER(gpio_interface, LOG_LEVEL_DBG);

// Private GPIO device specifications (defined here, not in header)
static const struct gpio_dt_spec ENC1SW = GPIO_DT_SPEC_GET(DT_ALIAS(enc1sw), gpios);
static const struct gpio_dt_spec ENC2SW = GPIO_DT_SPEC_GET(DT_ALIAS(enc2sw), gpios);
static const struct gpio_dt_spec ENC1A = GPIO_DT_SPEC_GET(DT_ALIAS(enc1a), gpios);
static const struct gpio_dt_spec ENC1B = GPIO_DT_SPEC_GET(DT_ALIAS(enc1b), gpios);
static const struct gpio_dt_spec ENC2A = GPIO_DT_SPEC_GET(DT_ALIAS(enc2a), gpios);
static const struct gpio_dt_spec ENC2B = GPIO_DT_SPEC_GET(DT_ALIAS(enc2b), gpios);

static const struct gpio_dt_spec BTN1 = GPIO_DT_SPEC_GET(DT_ALIAS(trig1), gpios);
static const struct gpio_dt_spec BTN2 = GPIO_DT_SPEC_GET(DT_ALIAS(trig2), gpios);
static const struct gpio_dt_spec BTN3 = GPIO_DT_SPEC_GET(DT_ALIAS(trig3), gpios);
static const struct gpio_dt_spec BTN4 = GPIO_DT_SPEC_GET(DT_ALIAS(trig4), gpios);
static const struct gpio_dt_spec BTN5 = GPIO_DT_SPEC_GET(DT_ALIAS(jb1), gpios);
static const struct gpio_dt_spec BTN6 = GPIO_DT_SPEC_GET(DT_ALIAS(jb2), gpios);
static const struct gpio_dt_spec BTN7 = GPIO_DT_SPEC_GET(DT_ALIAS(jb3), gpios);
static const struct gpio_dt_spec BTN8 = GPIO_DT_SPEC_GET(DT_ALIAS(jb4), gpios);
static const struct gpio_dt_spec PowerLED = GPIO_DT_SPEC_GET(DT_ALIAS(pwrled), gpios);

// Interrupt callback structures (private)
static struct gpio_callback input_cb_gpio0;
static struct gpio_callback input_cb_gpio1;

// Global encoder direction variables (accessible via extern)
enum encoder_dir enc1_dir = ENC_BAD;
enum encoder_dir enc2_dir = ENC_BAD;

// Private encoder state tracking
static bool last_enc1_state = false;
static bool curr_enc1_state = false;
static bool last_enc2_state = false;
static bool curr_enc2_state = false;
static enum encoder_dir curr_encoder_dir = ENC_BAD;

// Accessor functions for GPIO specs
const struct gpio_dt_spec* get_enc1sw_gpio(void) { return &ENC1SW; }
const struct gpio_dt_spec* get_enc2sw_gpio(void) { return &ENC2SW; }
const struct gpio_dt_spec* get_btn1_gpio(void) { return &BTN1; }
const struct gpio_dt_spec* get_btn2_gpio(void) { return &BTN2; }
const struct gpio_dt_spec* get_btn3_gpio(void) { return &BTN3; }
const struct gpio_dt_spec* get_btn4_gpio(void) { return &BTN4; }
const struct gpio_dt_spec* get_btn5_gpio(void) { return &BTN5; }
const struct gpio_dt_spec* get_btn6_gpio(void) { return &BTN6; }
const struct gpio_dt_spec* get_btn7_gpio(void) { return &BTN7; }
const struct gpio_dt_spec* get_btn8_gpio(void) { return &BTN8; }
const struct gpio_dt_spec* get_power_led_gpio(void) { return &PowerLED; }

// Accessor functions for callback structures
struct gpio_callback* get_gpio0_callback(void) { return &input_cb_gpio0; }
struct gpio_callback* get_gpio1_callback(void) { return &input_cb_gpio1; }

// Button initialization function
int ButtonsInit(void)
{
    const struct gpio_dt_spec* buttons[] = {&BTN1, &BTN2, &BTN3, &BTN4, &BTN5, &BTN6, &BTN7, &BTN8};
    const char* button_names[] = {"BTN1", "BTN2", "BTN3", "BTN4", "BTN5", "BTN6", "BTN7", "BTN8"};
    
    for (int i = 0; i < 8; i++) {
        if (!gpio_is_ready_dt(buttons[i])) {
            LOG_ERR("%s not ready", button_names[i]);
            return -1;
        }
        
        // Configure BTN7 and BTN8 as simple inputs (for polling)
        if (i >= 6) {
            if (gpio_pin_configure_dt(buttons[i], GPIO_INPUT) != 0) {
                LOG_ERR("Failed to configure %s as input", button_names[i]);
                return -1;
            }
        } else {
            // Configure BTN1-BTN6 with interrupt capability
            if (gpio_pin_configure_dt(buttons[i], GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0) {
                LOG_ERR("Failed to configure %s as input with interrupt", button_names[i]);
                return -1;
            }
        }
    }
    
    LOG_INF("All buttons initialized successfully");
    return 0;
}

// Encoder initialization function
int EncodersInit(void)
{
    const struct gpio_dt_spec* encoders[] = {&ENC1SW, &ENC2SW, &ENC1A, &ENC1B, &ENC2A, &ENC2B};
    const char* encoder_names[] = {"ENC1SW", "ENC2SW", "ENC1A", "ENC1B", "ENC2A", "ENC2B"};
    
    for (int i = 0; i < 6; i++) {
        if (!gpio_is_ready_dt(encoders[i])) {
            LOG_ERR("%s not ready", encoder_names[i]);
            return -1;
        }
        
        // Configure encoder switches with interrupts, A/B pins as simple inputs
        if (i < 2) {
            if (gpio_pin_configure_dt(encoders[i], GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0) {
                LOG_ERR("Failed to configure %s as input with interrupt", encoder_names[i]);
                return -1;
            }
        } else {
            if (gpio_pin_configure_dt(encoders[i], GPIO_INPUT) != 0) {
                LOG_ERR("Failed to configure %s as input", encoder_names[i]);
                return -1;
            }
        }
    }
    
    // Save initial encoder states
    curr_enc1_state = gpio_pin_get_dt(&ENC1A);
    curr_enc2_state = gpio_pin_get_dt(&ENC2A);
    last_enc1_state = curr_enc1_state;
    last_enc2_state = curr_enc2_state;
    
    LOG_INF("All encoders initialized successfully");
    return 0;
}

// Power LED initialization
int PWR_LED_Init(void)
{
    if (!gpio_is_ready_dt(&PowerLED)) {
        LOG_ERR("PowerLED not ready");
        return -1;
    }
    
    if (gpio_pin_configure_dt(&PowerLED, GPIO_OUTPUT_INACTIVE) != 0) {
        LOG_ERR("Failed to configure PowerLED as output");
        return -1;
    }
    
    LOG_INF("Power LED initialized successfully");
    return 0;
}

// Master GPIO initialization function
int GPIO_Init(void)
{
    int err_code = 0;
    
    LOG_INF("Initializing GPIO interface");
    
    err_code = ButtonsInit();
    if (err_code != 0) {
        LOG_ERR("Failed to initialize buttons: %d", err_code);
        return err_code;
    }
    
    err_code = EncodersInit();
    if (err_code != 0) {
        LOG_ERR("Failed to initialize encoders: %d", err_code);
        return err_code;
    }
    
    err_code = PWR_LED_Init();
    if (err_code != 0) {
        LOG_ERR("Failed to initialize PWR LED: %d", err_code);
        return err_code;
    }
    
    // Configure interrupt pins
    gpio_pin_interrupt_configure_dt(&ENC1SW, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&ENC2SW, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN1, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN2, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN3, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN4, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN5, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&BTN6, GPIO_INT_EDGE_BOTH);
    
    // Initialize GPIO callbacks
    gpio_init_callback(&input_cb_gpio0, input_interrupt_handler, 
                      BIT(ENC1SW.pin) | BIT(BTN5.pin) | BIT(BTN6.pin));
    gpio_init_callback(&input_cb_gpio1, input_interrupt_handler,
                      BIT(ENC2SW.pin) | BIT(BTN1.pin) | BIT(BTN2.pin) | 
                      BIT(BTN3.pin) | BIT(BTN4.pin));
    
    // Add callbacks to GPIO ports
    gpio_add_callback(ENC1SW.port, &input_cb_gpio0);
    gpio_add_callback(ENC2SW.port, &input_cb_gpio1);
    
    LOG_INF("GPIO interface initialized successfully");
    return 0;
}

// GPIO state reading functions
bool get_enc1_sw(void)
{
    return gpio_pin_get_dt(&ENC1SW);
}

bool get_enc2_sw(void)
{
    return gpio_pin_get_dt(&ENC2SW);
}

uint8_t get_enc1_dir(void)
{
    curr_enc1_state = gpio_pin_get_dt(&ENC1A);
    if (curr_enc1_state != last_enc1_state && curr_enc1_state == 0) {
        if (gpio_pin_get_dt(&ENC1B) != curr_enc1_state) {
            curr_encoder_dir = ENC_CW;
        } else {
            curr_encoder_dir = ENC_CCW;
        }
    } else {
        curr_encoder_dir = ENC_BAD;
    }
    last_enc1_state = curr_enc1_state;
    enc1_dir = curr_encoder_dir;  // Update global variable
    return curr_encoder_dir;
}

uint8_t get_enc2_dir(void)
{
    curr_enc2_state = gpio_pin_get_dt(&ENC2A);
    if (curr_enc2_state != last_enc2_state && curr_enc2_state == 0) {
        if (gpio_pin_get_dt(&ENC2B) != curr_enc2_state) {
            curr_encoder_dir = ENC_CW;
        } else {
            curr_encoder_dir = ENC_CCW;
        }
    } else {
        curr_encoder_dir = ENC_BAD;
    }
    last_enc2_state = curr_enc2_state;
    enc2_dir = curr_encoder_dir;  // Update global variable
    return curr_encoder_dir;
}

// Utility functions
bool read_gpio_pin(const struct gpio_dt_spec* gpio_spec)
{
    if (!gpio_spec) return false;
    return gpio_pin_get_dt(gpio_spec);
}

int set_gpio_pin(const struct gpio_dt_spec* gpio_spec, int value)
{
    if (!gpio_spec) return -EINVAL;
    return gpio_pin_set_dt(gpio_spec, value);
}

int toggle_gpio_pin(const struct gpio_dt_spec* gpio_spec)
{
    if (!gpio_spec) return -EINVAL;
    return gpio_pin_toggle_dt(gpio_spec);
}

bool is_button_pressed(const struct gpio_dt_spec* gpio_spec)
{
    if (!gpio_spec) return false;
    // Assuming active low buttons (pressed = 0, released = 1)
    return (gpio_pin_get_dt(gpio_spec) == 0);
}

uint32_t get_button_states(void)
{
    uint32_t states = 0;
    const struct gpio_dt_spec* buttons[] = {
        &BTN1, &BTN2, &BTN3, &BTN4, &BTN5, &BTN6, &BTN7, &BTN8
    };
    
    for (int i = 0; i < 8; i++) {
        if (is_button_pressed(buttons[i])) {
            states |= BIT(i);
        }
    }
    
    return states;
}

// LED control functions
int set_power_led(bool state)
{
    return gpio_pin_set_dt(&PowerLED, state ? 1 : 0);
}

int toggle_power_led(void)
{
    return gpio_pin_toggle_dt(&PowerLED);
}

// Button handlers for polling (BTN7 and BTN8)
void BTN7_Handler(fsm_struct* fsm)
{
    if (!gpio_pin_get_dt(&BTN7) && fsm->btn[6] == false && fsm->btn_change[6] == false) {
        fsm->btn_change[6] = true;
        fsm->btn[6] = true;
        LOG_INF("BTN7 = 1");
    } else if (gpio_pin_get_dt(&BTN7) && fsm->btn[6] == true && fsm->btn_change[6] == true) {
        fsm->btn_change[6] = false;
        fsm->btn[6] = false;
        LOG_INF("BTN7 = 0");
    }
}

void BTN8_Handler(fsm_struct* fsm)
{
    if (!gpio_pin_get_dt(&BTN8) && fsm->btn[7] == false && fsm->btn_change[7] == false) {
        fsm->btn_change[7] = true;
        fsm->btn[7] = true;
        LOG_INF("BTN8 = 1");
    } else if (gpio_pin_get_dt(&BTN8) && fsm->btn[7] == true && fsm->btn_change[7] == true) {
        fsm->btn_change[7] = false;
        fsm->btn[7] = false;
        LOG_INF("BTN8 = 0");
    }
}

bool track_new = false;

//Function to handle track/instrument/tempo selection
void ENC1_Handler(fsm_struct* fsm)
{
    if (fsm->settings_menu.music_settings != MUSIC_IDLE && fsm->settings_menu.operation_settings == OPERATION_IDLE)
    {
        //arp_stop();  //Pat note: copied from old SAMI, don't know what this is
        switch(fsm->settings_menu.music_settings)
        {
            case SET_TRACK:
                i2c_lcd_clear();
                i2c_lcd_draw_track(fsm->current_track);

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
                            i2c_lcd_draw_track(fsm->current_track);

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
                            i2c_lcd_draw_track(fsm->current_track);

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
                i2c_lcd_clear();
                i2c_lcd_draw_instrument(fsm->instrument);

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
                            i2c_lcd_draw_instrument(fsm->instrument);
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
                            i2c_lcd_draw_instrument(fsm->instrument);
                            break;
                    }
                }
                //TODO - SOMEONE: Replace this with function that sets instrument in midi file - not created yet
                //midi_set_instrument(0, VS1053_Instrument[fsm->instrument]);
                break;

            case SET_TEMPO:
                i2c_lcd_clear();
                i2c_lcd_draw_tempo(fsm->tempo);

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
                            i2c_lcd_draw_tempo(fsm->tempo);
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
                            i2c_lcd_draw_tempo(fsm->tempo);
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
                i2c_lcd_clear();
                i2c_lcd_draw_input(fsm->input_mode);
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

                            i2c_lcd_draw_input(fsm->input_mode);
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

                            i2c_lcd_draw_input(fsm->input_mode);
                            break;
                    }
                }

                break;
            
            case PLAYBACK_MENU:
                i2c_lcd_clear();
                i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);
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
                                    i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

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
                                    i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

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
                                    i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

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
                                    i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);

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
                                    i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);
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
                                    i2c_lcd_draw_playback(fsm->input_mode, fsm->play_mode);
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