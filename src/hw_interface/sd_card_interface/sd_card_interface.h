#ifndef SDCARD_INTERFACE_H
#define SDCARD_INTERFACE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "state_machine_defs.h"

// Use this as a guideline for your TODOs

/**
 * @brief Check for devices
 * 
 * This function checks the subsystem for all devices
 * 
 * @return int 0 on success, negative error code otherwise
 */
int CheckDevices(void);

/**
 * @brief Initialize the SD card interface
 * 
 * This function initializes the SD card interface hardware.
 * 
 * @return int 0 on success, negative error code otherwise
 */
int SDcardInterfaceInit(void);

/**
 * @brief Initialize the SD card
 * 
 * This function initializes the SD card file system and scans for MIDI files.
 * 
 * @return int 0 on success, negative error code otherwise
 */
int SDcardInit(void);

/**
 * @brief Scan for MIDI files
 * 
 * This function scans the MIDI directory for .midi files.
 * 
 * @return int Number of MIDI files found
 */
int scan_midi_files(void);

/**
 * @brief Get the number of MIDI tracks
 * 
 * @return uint8_t Number of tracks
 */
uint8_t get_track_count(void);

/**
 * @brief Get the file name for a track
 * 
 * @param track_number Track number (1-based)
 * @param file_name Buffer to store the file name
 * @param len Length of the buffer
 * @return int 0 on success, negative error code otherwise
 */
int get_track_file_name(uint8_t track_number, char *file_name, size_t len);

/**
 * @brief Read a file from the SD card
 * 
 * @param file_name File name
 * @param buffer Buffer to store the file contents
 * @param max_size Maximum size to read
 * @return int Number of bytes read, negative error code otherwise
 */
int read_file(const char *file_name, uint8_t *buffer, size_t max_size);

/**
 * @brief Write a file to the SD card
 * 
 * @param file_name File name
 * @param buffer Buffer containing data to write
 * @param size Size of data to write
 * @return int 0 on success, negative error code otherwise
 */
int write_file(const char *file_name, const uint8_t *buffer, size_t size);

/**
 * @brief Get the size of a file
 * 
 * @param file_name File name
 * @return int File size, negative error code otherwise
 */
int get_file_size(const char *file_name);

/**
 * @brief Read a MIDI file
 * 
 * @param file_name File name
 * @param buffer Buffer to store the file contents
 * @param max_size Maximum size to read
 * @return int Number of bytes read, negative error code otherwise
 */
int read_midi(const char *file_name, uint8_t *buffer, size_t max_size);

// These are for reading and writing settings - we can flesh this out once we are clear on what the FSM will be

/**
 * @brief Write settings to a file
 * 
 * @param fsm State machine
 * @return int 0 on success, negative error code otherwise
 */
int write_setting_txt(fsm_struct *fsm);

/**
 * @brief Read settings from a file
 * 
 * @param fsm State machine
 * @return int 0 on success, negative error code otherwise
 */
int read_settings_txt(fsm_struct *fsm);

#endif // SDCARD_INTERFACE_H
