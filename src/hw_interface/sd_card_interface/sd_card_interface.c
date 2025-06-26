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
#include <zephyr/fs/fs.h>
#include <diskio.h>
#include <string.h>
#include <strings.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "sd_card_interface.h"
#include "spi_interface.h"

#define MODULE sd_card_interface
LOG_MODULE_REGISTER(MODULE);

//
//Global variables
//
#define DISK_MOUNT_PT   "/SD:"
//#define DISK_NAME       "SD Card slot" //Must match the overlay label
//FATFS objects
static FATFS fs;
static DIR dir;
static FILINFO fno;
static FIL file;
//SD card logical drive
#define SD_DRIVE        "SD:"
//MIDI directory
#define MIDI_DIR        "/MIDI"
//Settings file
#define SETTINGS_FILE   "/settings.txt"
//Maximum number of tracks
#define MAX_MIDI_TRACKS     10
//Buffers
#define MAX_SD_READ_BUFFER  512
#define MAX_SETTINGS_READ_WRITE_BUFFER  32
static char file_buf[1024];
//Track file names
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
    //Small delay
    k_sleep(K_MSEC(500));

    int ret_code = 0;
    uint8_t retries = 3;

    //Check if SD spi is ready
    const struct device *sdc_spi = DEVICE_DT_GET(DT_ALIAS(sdcardspi));
    if (!device_is_ready(sdc_spi))
    {
        LOG_INF("SD - SPI not ready");
        return -1;
    }
    LOG_INF("SD - SPI ready");

    //Check if mmc is ready
    const struct device *sdc_disk = DEVICE_DT_GET(DT_ALIAS(sdcarddisk));
    if (!device_is_ready(sdc_disk))
    {
        LOG_INF("SD - MMC not ready");
        return -1;
    }
    LOG_INF("SD - MMC ready");

    //Check if disk is already initialized
    const char *disk_name = sdc_disk->name;
    LOG_INF("SD card mmc name: %s", disk_name);

    while (retries--) 
    {
        ret_code = disk_access_init(disk_name);
        if (ret_code == 0){
            LOG_INF("Disk init success - test 1\n");
            return 0;
        }
        else
        {
            LOG_INF("%s Disk init failed, code: %d, retrying\n", disk_name, ret_code);
            k_sleep(K_MSEC(500));
        }
    }
    LOG_INF("Disk init failed three times\n");
    return -1;
}

int CheckDevices(void)
{
    const struct device *device_list;
    size_t device_count;

    device_count = z_device_get_all_static(&device_list);
    LOG_INF("Total devices found: %d", device_count);
    if (device_count == 0){
        return -1;
    }

    for (size_t i = 0; i < device_count; i++)
    {
        const struct device *curr_dev = &device_list[i];
        LOG_INF("Device %d name: %s", i, curr_dev->name);
        k_sleep(K_MSEC(1000));
    }
    LOG_INF("End of Device list\n");
    return 0;
}

/**
 * @brief Initialize the SD card file system
 * 
 * @return int 0 on success, negative error code otherwise
 */

 // TODO - Patrick: Check all these

int SDcardInit(void) {
    struct fs_mount_t mp = {
        .type = FS_FATFS,
        .fs_data = &fs,
        .mnt_point = DISK_MOUNT_PT,
    };
    
    //Mount the file system
    int ret = fs_mount(&mp);

    if (ret != 0) {
        LOG_INF("Mounting failed\n");
        return -1;
    }
    
    LOG_INF("SD card mounted successfully");

    struct fs_dir_t dir;
    struct fs_dirent entry;

    //Initialize directory
    fs_dir_t_init(&dir);

    //Try to open the directory
    ret = fs_opendir(&dir, DISK_MOUNT_PT);
    if (ret != 0) {
        LOG_INF("Failed to open root dir: %d", ret);
        return -1;
    }

    //Display directory contents
    LOG_INF("Contents of root directory:");
    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != 0) {
        if (entry.type == FS_DIR_ENTRY_DIR) {
            LOG_INF("<DIR>   %s", entry.name);
        } else {
            LOG_INF("%9u  %s", entry.size, entry.name);
        }
    }

    fs_closedir(&dir);

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
        LOG_INF("Failed to open MIDI directory: %d", res);
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
        LOG_INF("Failed to open file %s: %d", file_name, res);
        return -EIO;
    }
    
    /* Read file contents */
    res = f_read(&file, buffer, max_size, &bytes_read);
    if (res != FR_OK) {
        LOG_INF("Failed to read file %s: %d", file_name, res);
        f_close(&file);
        return -EIO;
    }
    
    /* Close file */
    f_close(&file);
    
    LOG_INF("Read %d bytes from file %s", bytes_read, file_name);
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
        LOG_INF("Failed to open file %s for writing: %d", file_name, res);
        return -EIO;
    }
    
    /* Write file contents */
    res = f_write(&file, buffer, size, &bytes_written);
    if (res != FR_OK || bytes_written != size) {
        LOG_INF("Failed to write file %s: %d", file_name, res);
        f_close(&file);
        return -EIO;
    }
    
    /* Close file */
    f_close(&file);
    
    LOG_INF("Wrote %d bytes to file %s", bytes_written, file_name);
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
int write_settings_txt(fsm_struct *fsm) 
{
    return 0;
}

/**
 * @brief Read settings from a file
 * 
 * @param fsm State machine
 * @return int 0 on success, negative error code otherwise
 */
int read_settings_txt(fsm_struct *fsm) 
{
    return 0;
}
