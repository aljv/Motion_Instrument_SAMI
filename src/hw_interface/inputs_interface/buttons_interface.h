#ifndef BUTTONS_INTERFACE_H
#define BUTTONS_INTERFACE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// TODO - Patrick: start buttons interface code. Use interrupts to read from 6 buttons. Find a way
//                 to poll 2 buttons.
// Guide: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-2-reading-buttons-and-controlling-leds/
// Might not need this H file. Only use this if you think there are things in here that need to be accesed
// in different files of the code.

//So we can use these variables in main.c
extern const struct gpio_dt_spec ENC1SW;
extern const struct gpio_dt_spec ENC2SW;
extern const struct gpio_dt_spec BTN1;
extern const struct gpio_dt_spec BTN2;
extern const struct gpio_dt_spec BTN3;
extern const struct gpio_dt_spec BTN4;
extern const struct gpio_dt_spec BTN5;
extern const struct gpio_dt_spec BTN6;
extern const struct gpio_dt_spec BTN7;
extern const struct gpio_dt_spec BTN8;

enum encoder_dir {
    ENC_CW = 0,
    ENC_CCW,
    ENC_BAD,
};

/**
 * @brief Initialize the input buttons
 * 
 * First checks if all button gpios are ready, then configures them all as inputs.
 * 
 * @return int 0 on success, negative error code otherwise
 */
int ButtonsInit(void);

/**
 * @brief Initialize the encoders
 * 
 * First checks if all encoder gpios are ready, then configures them all as inputs.
 * 
 * @return int 0 on success, negative error code otherwise
 */
int EncodersInit(void);

/**
 * @brief Checks if ENC1 has been pressed
 * 
 * This function checks if encoder 1 switch has been triggered
 * 
 * @return boolean True if pressed, False otherwise
 */
bool get_enc1_sw(void);

/**
 * @brief Checks if ENC2 has been pressed
 * 
 * This function checks if ENC2 switch has been triggered
 * 
 * @return boolean True if pressed, False otherwise
 */
bool get_enc2_sw(void);

/**
 * @brief Checks direction of ENC1
 * 
 * This function checks the direction of ENC1
 * 
 * @return "ENC_CCW" , "ENC_CW" , or "ENC_BAD"
 */
uint8_t get_enc1_dir(void);

/**
 * @brief Checks direction of ENC2
 * 
 * This function checks the direction of ENC2
 * 
 * @return "ENC_CCW" , "ENC_CW" , or "ENC_BAD"
 */
uint8_t get_enc2_dir(void);


#endif // BUTTONS_INTERFACE_H
