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
    LOG_INF("LED turned on");

    //VS1053Init();
    VS1053TestSPI();

    while (1) {
        dk_set_led_off(DK_LED1);
        k_sleep(K_SECONDS(1));
    }

    return 0;

}

