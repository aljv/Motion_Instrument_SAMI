#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#define MODULE encoder_interface
LOG_MODULE_REGISTER(MODULE);

// TODO - Patrick: start encoder interface code. Use interrupts to read from encoder inputs
//                 make sure rotation works.
// Guide: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-2-reading-buttons-and-controlling-leds/
// Might not need this H file. Only use this if you think there are things in here that need to be accesed
// in different files of the code.
