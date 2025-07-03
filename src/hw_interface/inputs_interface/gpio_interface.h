#ifndef GPIO_INTERFACE_H
#define GPIO_INTERFACE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "state_machine_defs.h"
#include "i2c_interface.h"
#include "midi.h"

// TODO - Patrick: start buttons interface code. Use interrupts to read from 6 buttons. Find a way
//                 to poll 2 buttons.
// Guide: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-2-reading-buttons-and-controlling-leds/
// Might not need this H file. Only use this if you think there are things in here that need to be accesed
// in different files of the code.

#define MAX_NUM_BTNS    8
#define NUM_ENCODERS    2

// GPIO accessor functions - these return pointers to the GPIO specs
const struct gpio_dt_spec* get_enc1sw_gpio(void);
const struct gpio_dt_spec* get_enc2sw_gpio(void);
const struct gpio_dt_spec* get_btn1_gpio(void);
const struct gpio_dt_spec* get_btn2_gpio(void);
const struct gpio_dt_spec* get_btn3_gpio(void);
const struct gpio_dt_spec* get_btn4_gpio(void);
const struct gpio_dt_spec* get_btn5_gpio(void);
const struct gpio_dt_spec* get_btn6_gpio(void);
const struct gpio_dt_spec* get_btn7_gpio(void);
const struct gpio_dt_spec* get_btn8_gpio(void);
const struct gpio_dt_spec* get_power_led_gpio(void);

// Button and encoder state functions
bool get_enc1_sw(void);
bool get_enc2_sw(void);
uint8_t get_enc1_dir(void);
uint8_t get_enc2_dir(void);

enum encoder_dir {
    ENC_CW = 0,
    ENC_CCW,
    ENC_BAD,
};

// Interrupt handler function (for use in ui_thread.c)
void input_interrupt_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins);

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
 * @brief Calls all initliazation functions
 * 
 * Initiliazes all inputs and checks if device is ready
 * 
 * @return 0 on success, -1 on fail
 */
int GPIO_Init(void);

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

/**
 * @brief Initialize PWR LED pin
 * 
 * This function initializes the Power LED pin
 * 
 * @return 0 on success, -1 on fail
 */
int PWR_LED_Init(void);

/**
 * @brief handles polling for buttons 7 and 8
 * 
 * This function handles button press for button 7 and 8
 * 
 * @return 
 */
void BTN7_Handler(fsm_struct* fsm);
void BTN8_Handler(fsm_struct* fsm);

/**
 * @brief handles encoder rotation and press
 * 
 * Button press switches menu on SAMI screen, encoder changes mode
 * 
 * @return 0 on success, -1 on fail
 */
void ENC1_Handler(fsm_struct* fsm);
void ENC2_Handler(fsm_struct* fsm);


#endif // BUTTONS_INTERFACE_H
