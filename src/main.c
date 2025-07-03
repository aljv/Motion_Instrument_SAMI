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
#include "hw_interface/VS1053_interface/VS1053_interface.h"
#include "hw_interface/VS1053_interface/midi.h"
#include "hw_interface/spi_interface.h"
#include "hw_interface/i2c_interface.h"
#include "hw_interface/uart_interface.h"
// TODO - Patrick: IMPORTANT
//                 this include has to be changed to state_machine.h once the file is changed
//    
#include "hw_interface/inputs_interface/gpio_interface.h"

  
#include "state_machine_defs.h"

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
#define MODULE main
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

//
//Function declarations
//

//TODO - Someone: Write these functions somewhere in main
    //void play_track(struct midi_track* track_list, enum playback_states playbackState, uint32_t current_time);
    //void all_playback_notes_off(void);


//ADC & battery charging
#define CHRG_PT   725 //needs charging
#define FULL_CHRG 835 //fully charged
bool charge_bat = false;
int adc = 0;
//MIDI
uint8_t midi_data[MAX_MIDI_DATA_LENGTH];


//
//Main function
//
int main(void)
{
    LOG_INF("Starting SAMI Motion Instrument application");
  
    

    LOG_INF("Initializing UART interface..");
    app_uart_init();
        
    // Initialize I2C interface
    LOG_INF("Initializing I2C interface...");
    i2c_interface_init();

    LOG_INF("Initializing VS1053 codec...");
    VS1053Init();
    k_msleep(2000);
  
    // Initialize audio amplifier GPIO control pins
    LOG_INF("Initializing audio amplifier GPIO...");
    int ret = audio_amplifier_gpio_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize audio amplifier GPIO");
        return ret;
    }

    //audio_amplifier_hardware_enable();
    // Initialize audio amplifier via I2C
    LOG_INF("Initializing MAX9744 audio amplifier...");
    max9744_set_volume(DEFAULT_AMP_VOL);
    k_msleep(100);
    audio_amplifier_hardware_enable();
  
    LOG_INF("Initializing Inputs and Encoders...");
    GPIO_Init();

    //AppTimer_Init();
    //AppTimer_Start();
    //Saadc_Init();
    
    LOG_INF("Initializing SD Card...");
    SDcardInterfaceInit(); //Error here when attempting to run - see sd_card_interface.c

    
    //TESTING PURPOSES - This works (PWR LED is red)
    /*LOG_INF("Setting PWR LED");
    gpio_pin_set_dt(&PowerLED, 1);*/
  
    //check_vs1053_audio_output();
    // Set VS1053 volume

    VS1053UpdateVolume(0x30, 0x30);
    midiSetChannelBank(0, 0x79);
    midiSetChannelVolume(0, 100);    
    midiSetInstrument(0, ELECTRIC_GRAND_PIANO);

    k_msleep(250);
    run_midi_tests();
  
    

    while (1)
    {
        //dk_set_led_off(DK_LED1);
        //k_sleep(K_SECONDS(1));

    }

    return 0;
}

