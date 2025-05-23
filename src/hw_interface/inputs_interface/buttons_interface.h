#ifndef BUTTONS_INTERFACE_H
#define BUTTONS_INTERFACE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

// TODO - Patrick: start buttons interface code. Use interrupts to read from 6 buttons. Find a way
//                 to poll 2 buttons.
// Guide: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-2-reading-buttons-and-controlling-leds/
// Might not need this H file. Only use this if you think there are things in here that need to be accesed
// in different files of the code.

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


#endif // VS1053_INTERFACE_H
