

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include "buttons_interface.h"

#define MODULE buttons_interface
LOG_MODULE_REGISTER(MODULE);

// TODO - Patrick: start buttons interface code. Use interrupts to read from 6 buttons. Find a way
//                 to poll 2 buttons.
// Guide: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-2-reading-buttons-and-controlling-leds/

//
//Create gpio_dt_spec structs for each gpio pin
//
//Encoders
const struct gpio_dt_spec ENC1SW = GPIO_DT_SPEC_GET(DT_ALIAS(enc1sw), gpios);
const struct gpio_dt_spec ENC2SW = GPIO_DT_SPEC_GET(DT_ALIAS(enc2sw), gpios);
static const struct gpio_dt_spec ENC1A = GPIO_DT_SPEC_GET(DT_ALIAS(enc1a), gpios);
static const struct gpio_dt_spec ENC1B = GPIO_DT_SPEC_GET(DT_ALIAS(enc1b), gpios);
static const struct gpio_dt_spec ENC2A = GPIO_DT_SPEC_GET(DT_ALIAS(enc2a), gpios);
static const struct gpio_dt_spec ENC2B = GPIO_DT_SPEC_GET(DT_ALIAS(enc2b), gpios);
//Buttons - note the change in name from TRIG & JB to BTN
const struct gpio_dt_spec BTN1 = GPIO_DT_SPEC_GET(DT_ALIAS(trig1), gpios);
const struct gpio_dt_spec BTN2 = GPIO_DT_SPEC_GET(DT_ALIAS(trig2), gpios);
const struct gpio_dt_spec BTN3 = GPIO_DT_SPEC_GET(DT_ALIAS(trig3), gpios);
const struct gpio_dt_spec BTN4 = GPIO_DT_SPEC_GET(DT_ALIAS(trig4), gpios);
const struct gpio_dt_spec BTN5 = GPIO_DT_SPEC_GET(DT_ALIAS(jb1), gpios);
const struct gpio_dt_spec BTN6 = GPIO_DT_SPEC_GET(DT_ALIAS(jb2), gpios);
const struct gpio_dt_spec BTN7 = GPIO_DT_SPEC_GET(DT_ALIAS(jb3), gpios);
const struct gpio_dt_spec BTN8 = GPIO_DT_SPEC_GET(DT_ALIAS(jb4), gpios);
//PWR LED pin
const struct gpio_dt_spec PowerLED = GPIO_DT_SPEC_GET(DT_ALIAS(pwrled), gpios);

//global variables to hold current and previous state of encoders
bool last_enc1_state;
bool curr_enc1_state;
bool last_enc2_state;
bool curr_enc2_state;

//this variable holds the extrapolated encoder direction
enum encoder_dir curr_encoder_dir;

int ButtonsInit(void){
    if (!gpio_is_ready_dt(&BTN1)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN1, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN2)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN2, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN3)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN3, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN4)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN4, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN5)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN5, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN6)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN6, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN7)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN7, GPIO_INPUT) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&BTN8)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&BTN8, GPIO_INPUT) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }
    
    return 0;
}

int EncodersInit(void){
    if (!gpio_is_ready_dt(&ENC1SW)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&ENC1SW, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&ENC2SW)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&ENC2SW, GPIO_INPUT | GPIO_INT_EDGE_BOTH) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&ENC1A)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&ENC1A, GPIO_INPUT) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&ENC1B)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&ENC1B, GPIO_INPUT) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&ENC2A)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&ENC2A, GPIO_INPUT) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    if (!gpio_is_ready_dt(&ENC2B)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&ENC2B, GPIO_INPUT) != 0){
        LOG_ERR("Failed to configure ENC1SW as input\n");
        return -1;
    }

    //Save current encoder states
    curr_enc1_state = gpio_pin_get_dt(&ENC1A);
    curr_enc2_state = gpio_pin_get_dt(&ENC2A);

    return 0;
}

bool get_enc1_sw(void){
    return gpio_pin_get_dt(&ENC1SW);
}

bool get_enc2_sw(void){
    return gpio_pin_get_dt(&ENC2SW);
}

uint8_t get_enc1_dir(void){
    curr_enc1_state = gpio_pin_get_dt(&ENC1A);
    if (curr_enc1_state != last_enc1_state && curr_enc1_state == 0){
        if(gpio_pin_get_dt(&ENC1B) != curr_enc1_state){
            curr_encoder_dir = ENC_CW;
        }else{
            curr_encoder_dir = ENC_CCW;
        }
    }else{
        curr_encoder_dir = ENC_BAD;
    }
    last_enc1_state = curr_enc1_state;
    return curr_encoder_dir;
}

uint8_t get_enc2_dir(void){
    curr_enc2_state = gpio_pin_get_dt(&ENC2A);
    if (curr_enc2_state != last_enc2_state && curr_enc2_state == 0){
        if(gpio_pin_get_dt(&ENC2B) != curr_enc2_state){
            curr_encoder_dir = ENC_CW;
        }else{
            curr_encoder_dir = ENC_CCW;
        }
    }else{
        curr_encoder_dir = ENC_BAD;
    }
    last_enc2_state = curr_enc2_state;
    return curr_encoder_dir;
}

int PWR_LED_Init(void){
    if (!gpio_is_ready_dt(&PowerLED)){
        LOG_ERR("ENC1SW not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&PowerLED, GPIO_OUTPUT_INACTIVE) != 0){
        LOG_ERR("Failed to configure BATADC as input\n");
        return -1;
    }
    return 0;
}