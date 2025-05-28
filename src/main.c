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
#include "hw_interface/spi_interface.h"
#include "hw_interface/i2c_interface.h"
#include "hw_interface/inputs_interface/buttons_interface.h"

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
#define MODULE main
LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);

void input_interrupt_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins);
static struct gpio_callback input_cb_gpio0;
static struct gpio_callback input_cb_gpio1;

// TODO - Patrick: this function will call the encoder and button init functions (stored in buttons_interface.c)
//                 as well as init. those pins with the general interrupt handler (function has not been written
//                 yet, will copy from old SAMI)
// 
void GPIO_Init(void){
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
    gpio_init_callback(&input_cb_gpio0, input_interrupt_handler, BIT(ENC1SW.pin)
                                                                | BIT(BTN5.pin)
                                                                | BIT(BTN6.pin));
    //Initialize the gpio1 port callback
    gpio_init_callback(&input_cb_gpio1, input_interrupt_handler, BIT(ENC2SW.pin)
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

// TODO - Patrick: this is the general prupose interrupt handler function for BTN1-6 and both encoders
//                 BTN7 and BTN8 are handled seperately
void input_interrupt_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins){
    if ((pins & BIT(ENC1SW.pin)) && port == ENC1SW.port){
        //ENC1SW triggered
        LOG_INF("ENC1 triggered\n");
    }
    else if ((pins & BIT(ENC2SW.pin)) && port == ENC2SW.port){
        //ENC2SW triggered
        LOG_INF("ENC2 triggered\n");
    }
    else if ((pins & BIT(BTN1.pin)) && port == BTN1.port){
        //BTN1 triggered
        LOG_INF("BTN1 triggered\n");
    }
    else if ((pins & BIT(BTN2.pin)) && port == BTN2.port){
        //BTN2 triggered
        LOG_INF("BTN2 triggered\n");
    }
    else if ((pins & BIT(BTN3.pin)) && port == BTN3.port){
        //BTN3 triggered
        LOG_INF("BTN3 triggered\n");
    }
    else if ((pins & BIT(BTN4.pin)) && port == BTN4.port){
        //BTN4 triggered
        LOG_INF("BTN4 triggered\n");
    }
    else if ((pins & BIT(BTN5.pin)) && port == BTN5.port){
        //BTN5 triggered
        LOG_INF("BTN5 triggered\n");
    }
    else if ((pins & BIT(BTN6.pin)) && port == BTN6.port){
        //BTN6 triggered
        LOG_INF("BTN6 triggered\n");
    }
}

int main(void)
{
    GPIO_Init();
    LOG_INF("GPIO INIT complete");
    SDcardInit(); //Error here when attempting to run - see sd_card_interface.c
    dk_leds_init();
    dk_set_led_on(DK_LED1);
    LOG_INF("LED turned on");

    while (1) {
        dk_set_led_off(DK_LED1);
        k_sleep(K_SECONDS(1));
    }

    return 0;

}

