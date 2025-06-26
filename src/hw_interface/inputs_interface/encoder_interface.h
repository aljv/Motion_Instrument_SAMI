#ifndef ENCODER_INTERFACE_H
#define ENCODER_INTERFACE_H

//
//NOTE
//      THIS FILE IS ACTUALLY THE STATE_MACHINE FILE, NAME HAS TO BE CHANGED STILL
//      AS RIGHT NOW IT IS encoder_interface.h

#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

// TODO - Patrick: This file is actually going to be the state_machine file. Name needs
//                 to be changed and CMake has to be changed as well.

#define MAX_NUM_BTNS    8
#define NUM_ENCODERS    2


#endif // ENCODER_INTERFACE_H
