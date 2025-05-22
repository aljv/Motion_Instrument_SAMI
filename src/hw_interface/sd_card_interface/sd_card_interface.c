/**
 * @file app_sd.c
 * @brief SD card interface for SAMI MIDI instrument
 *
 * This file contains the implementation of the SD card interface
 * for the SAMI MIDI instrument using the FATFS file system.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <diskio.h>
#include <string.h>

#include "sd_card_interface.h"
#include "spi_interface.h"

#define MODULE sd_card_interface
LOG_MODULE_REGISTER(MODULE);

/* FATFS objects */
static FATFS fs;
static DIR dir;
static FILINFO fno;
static FIL file;

/* SD card logical drive */
#define SD_DRIVE "SD:"

/* MIDI directory */
#define MIDI_DIR "/MIDI"

/* Settings file */
#define SETTINGS_FILE "/settings.txt"

/* Maximum number of tracks */
#define MAX_MIDI_TRACKS 10

/* Buffer for file operations */
static char file_buf[1024];

/* Track file names */
static char track_file_names[MAX_MIDI_TRACKS][32];
static uint8_t num_tracks = 0;

// TODO PATRICK - This is code spat out by chat GPT after I put in SEGGER code into it and asked it to convert to NRF Connect
//              - test all these functions out in main to see if it works and change accordingly. Uses FATFS

/**
 * @brief Initialize the SD card interface hardware
 * 
 * @return int 0 on success, negative error code otherwise
 */
int SDcardInterfaceInit(void) {
    LOG_INF("Initializing SD card interface hardware");
    
    /* Initialize SPI for SD card */
    /* Note: This assumes SPI initialization is done elsewhere */
    
    return 0;
}

/**
 * @brief Initialize the SD card file system
 * 
 * @return int 0 on success, negative error code otherwise
 */

 // TODO - First test in main: <err> sd_card_interface: Failed to initialize disk. idk why look into it lmao

int SDcardInit(void) {
    FRESULT res;
    
    /* Initialize disk */
    if (disk_initialize(0) != RES_OK) {
        LOG_ERR("Failed to initialize disk");
        return -EIO;
    }
    
    /* Mount the file system */
    res = f_mount(&fs, SD_DRIVE, 1);
    if (res != FR_OK) {
        LOG_ERR("Failed to mount file system: %d", res);
        return -EIO;
    }
    
    LOG_INF("SD card mounted successfully");
    
    /* Create MIDI directory if it doesn't exist */
    char midi_path[32];
    snprintf(midi_path, sizeof(midi_path), "%s%s", SD_DRIVE, MIDI_DIR);
    
    res = f_stat(midi_path, &fno);
    if (res != FR_OK) {
        /* Directory doesn't exist, create it */
        res = f_mkdir(midi_path);
        if (res != FR_OK) {
            LOG_ERR("Failed to create MIDI directory: %d", res);
            return -EIO;
        }
        LOG_INF("Created MIDI directory");
    }
    
    /* Scan for MIDI files */
    scan_midi_files();
    
    return 0;
}

/**
 * @brief Scan for MIDI files in the MIDI directory
 * 
 * This function scans the MIDI directory for .mid files and
 * populates the track_file_names array.
 * 
 * @return int Number of MIDI files found
 */
int scan_midi_files(void) {
    FRESULT res;
    char midi_path[32];
    
    /* Reset track count */
    num_tracks = 0;
    
    /* Format MIDI directory path */
    snprintf(midi_path, sizeof(midi_path), "%s%s", SD_DRIVE, MIDI_DIR);
    
    /* Open MIDI directory */
    res = f_opendir(&dir, midi_path);
    if (res != FR_OK) {
        LOG_ERR("Failed to open MIDI directory: %d", res);
        return 0;
    }
    
    /* Scan for MIDI files */
    while (1) {
        /* Read directory entry */
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            /* End of directory or error */
            break;
        }
        
        /* Skip directories */
        if (fno.fattrib & AM_DIR) {
            continue;
        }
        
        /* Check if file has .mid extension */
        size_t name_len = strlen(fno.fname);
        if (name_len > 4 && strcasecmp(&fno.fname[name_len - 4], ".mid") == 0) {
            /* Copy file name to track_file_names array */
            if (num_tracks < MAX_MIDI_TRACKS) {
                snprintf(track_file_names[num_tracks], sizeof(track_file_names[0]),
                         "%s/%s", midi_path, fno.fname);
                num_tracks++;
            }
        }
    }
    
    /* Close directory */
    f_closedir(&dir);
    
    LOG_INF("Found %d MIDI files", num_tracks);
    return num_tracks;
}

/**
 * @brief Get the number of MIDI tracks
 * 
 * @return uint8_t Number of tracks
 */
uint8_t get_track_count(void) {
    return num_tracks;
}

/**
 * @brief Get the file name for a track
 * 
 * @param track_number Track number (1-based)
 * @param file_name Buffer to store the file name
 * @param len Length of the buffer
 * @return int 0 on success, negative error code otherwise
 */
int get_track_file_name(uint8_t track_number, char *file_name, size_t len) {
    if (track_number == 0 || track_number > num_tracks) {
        return -EINVAL;
    }
    
    /* Copy file name to buffer */
    strncpy(file_name, track_file_names[track_number - 1], len);
    return 0;
}

/**
 * @brief Read a file from the SD card
 * 
 * @param file_name File name
 * @param buffer Buffer to store the file contents
 * @param max_size Maximum size to read
 * @return int Number of bytes read, negative error code otherwise
 */
int read_file(const char *file_name, uint8_t *buffer, size_t max_size) {
    FRESULT res;
    UINT bytes_read;
    
    /* Open file */
    res = f_open(&file, file_name, FA_READ);
    if (res != FR_OK) {
        LOG_ERR("Failed to open file %s: %d", file_name, res);
        return -EIO;
    }
    
    /* Read file contents */
    res = f_read(&file, buffer, max_size, &bytes_read);
    if (res != FR_OK) {
        LOG_ERR("Failed to read file %s: %d", file_name, res);
        f_close(&file);
        return -EIO;
    }
    
    /* Close file */
    f_close(&file);
    
    LOG_DBG("Read %d bytes from file %s", bytes_read, file_name);
    return bytes_read;
}

/**
 * @brief Write a file to the SD card
 * 
 * @param file_name File name
 * @param buffer Buffer containing data to write
 * @param size Size of data to write
 * @return int 0 on success, negative error code otherwise
 */
int write_file(const char *file_name, const uint8_t *buffer, size_t size) {
    FRESULT res;
    UINT bytes_written;
    
    /* Open file */
    res = f_open(&file, file_name, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        LOG_ERR("Failed to open file %s for writing: %d", file_name, res);
        return -EIO;
    }
    
    /* Write file contents */
    res = f_write(&file, buffer, size, &bytes_written);
    if (res != FR_OK || bytes_written != size) {
        LOG_ERR("Failed to write file %s: %d", file_name, res);
        f_close(&file);
        return -EIO;
    }
    
    /* Close file */
    f_close(&file);
    
    LOG_DBG("Wrote %d bytes to file %s", bytes_written, file_name);
    return 0;
}

/**
 * @brief Get the size of a file
 * 
 * @param file_name File name
 * @return int File size, negative error code otherwise
 */
int get_file_size(const char *file_name) {
    FRESULT res;
    
    /* Get file information */
    res = f_stat(file_name, &fno);
    if (res != FR_OK) {
        LOG_ERR("Failed to get file information for %s: %d", file_name, res);
        return -EIO;
    }
    
    return fno.fsize;
}

/**
 * @brief Read a MIDI file
 * 
 * @param file_name File name
 * @param buffer Buffer to store the file contents
 * @param max_size Maximum size to read
 * @return int Number of bytes read, negative error code otherwise
 */
int read_midi(const char *file_name, uint8_t *buffer, size_t max_size) {
    char full_path[64];
    
    /* Check if file_name already contains MIDI directory */
    if (strncmp(file_name, SD_DRIVE, strlen(SD_DRIVE)) == 0) {
        /* File name already contains path */
        strncpy(full_path, file_name, sizeof(full_path));
    } else {
        /* Add SD drive and MIDI directory to file name */
        snprintf(full_path, sizeof(full_path), "%s%s/%s", SD_DRIVE, MIDI_DIR, file_name);
    }
    
    /* Read file */
    return read_file(full_path, buffer, max_size);
}

/**
 * @brief Write settings to a file
 * 
 * @param fsm State machine
 * @return int 0 on success, negative error code otherwise
 */
/*int write_settings_txt(fsm_t *fsm) 
{
    
}*/

/**
 * @brief Read settings from a file
 * 
 * @param fsm State machine
 * @return int 0 on success, negative error code otherwise
 */
/*int read_settings_txt(fsm_t *fsm) 
{
    
}*/
