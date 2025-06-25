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

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <bluetooth/services/hrs_client.h>

#include "hw_interface/ble_interface/ble_interface.h"
#include "hw_interface/sd_card_interface/sd_card_interface.h"
#include "hw_interface/VS1053_interface/VS1053_interface.h"
#include "hw_interface/spi_interface.h"
#include "hw_interface/i2c_interface.h"
#include "hw_interface/VS1053_interface/midi.h"
#include "hw_interface/uart_interface.h"

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
#define MODULE main
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

int main(void)
{
    dk_leds_init();
    dk_set_led_on(DK_LED1);

    int ret;
        
    LOG_INF("Starting SAMI Motion Instrument application");

    LOG_INF("Initializing UART interface..");
    app_uart_init();
        
    // Initialize I2C interface
    LOG_INF("Initializing I2C interface...");
    i2c_interface_init();

    LOG_INF("Initializing VS1053 codec...");
    VS1053Init();
    k_msleep(2000);
    //check_midi_plugin_loaded();-
    //k_msleep(2000);
    //vs1053_register_test_suite();
    k_msleep(100);

    // Initialize audio amplifier GPIO control pins
    LOG_INF("Initializing audio amplifier GPIO...");
    ret = audio_amplifier_gpio_init();
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


    //check_vs1053_audio_output();
    // Set VS1053 volume

    VS1053UpdateVolume(0x30, 0x30);
    midiSetChannelBank(0, 0x79);
    midiSetChannelVolume(0, 100);    
    midiSetInstrument(0, ELECTRIC_GRAND_PIANO);

    k_msleep(250);
    run_midi_tests();

    while (1) {
        dk_set_led_off(DK_LED1);
        k_sleep(K_SECONDS(1));
    }

    return 0;

}

