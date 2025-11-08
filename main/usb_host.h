/**
 * @file usb_host.h
 * @brief USB Host Mode Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 6.0.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Partition table types
 */
typedef enum {
    PARTITION_TABLE_NONE = 0,      /**< No partition table detected */
    PARTITION_TABLE_MBR,            /**< MBR partition table */
    PARTITION_TABLE_GPT,            /**< GPT partition table */
    PARTITION_TABLE_UNKNOWN         /**< Unknown partition table */
} partition_table_type_t;

/**
 * @brief Partition information structure
 */
typedef struct {
    uint8_t boot_indicator;         /**< 0x80 = bootable, 0x00 = non-bootable */
    uint8_t partition_type;         /**< Partition type code */
    uint32_t start_lba;             /**< Starting LBA */
    uint32_t size_sectors;          /**< Size in sectors */
    uint64_t size_bytes;            /**< Size in bytes */
} partition_info_t;

/**
 * @brief Initialize USB Host Mode with MSC support
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_init(void);

/**
 * @brief Deinitialize USB Host Mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_deinit(void);

/**
 * @brief Check if USB device is connected
 * @return true if device connected, false otherwise
 */
bool usb_host_is_device_connected(void);

/**
 * @brief Check if USB Host is initialized
 * @return true if initialized, false otherwise
 */
bool usb_host_is_initialized(void);

/**
 * @brief Get USB mount point
 * @return Mount point path if USB drive is mounted, NULL otherwise
 */
const char* usb_host_get_mount_point(void);

/**
 * @brief Sync filesystem to ensure all data is written to USB drive
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_sync_filesystem(void);

/**
 * @brief Safely eject USB drive (sync + unmount + close)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_safe_eject(void);

/**
 * @brief Request safe eject (non-blocking)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_request_safe_eject(void);

/**
 * @brief Check if safe eject is requested
 * @return true if eject requested, false otherwise
 */
bool usb_host_is_eject_requested(void);

/**
 * @brief Read file from USB drive
 * @param file_path Path to file on USB drive (relative to mount point)
 * @param buffer Buffer to store file contents
 * @param buffer_size Size of buffer
 * @param bytes_read Pointer to store number of bytes read
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_read_file(const char* file_path, char* buffer, size_t buffer_size, size_t* bytes_read);

/**
 * @brief Write file to USB drive
 * @param file_path Path to file on USB drive (relative to mount point)
 * @param data Data to write
 * @param data_size Size of data to write
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_write_file(const char* file_path, const char* data, size_t data_size);

/**
 * @brief Get file size
 * @param file_path Path to file on USB drive (relative to mount point)
 * @param file_size Pointer to store file size
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_get_file_size(const char* file_path, size_t* file_size);

/**
 * @brief Read raw sector from USB drive
 * @param sector_num Sector number (LBA)
 * @param buffer Buffer to store sector data (must be at least 512 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_read_sector(uint32_t sector_num, uint8_t* buffer);

/**
 * @brief Write raw sector to USB drive
 * @param sector_num Sector number (LBA)
 * @param buffer Buffer containing sector data (must be 512 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_write_sector(uint32_t sector_num, const uint8_t* buffer);

/**
 * @brief Detect partition table type
 * @param table_type Pointer to store partition table type
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_detect_partition_table(partition_table_type_t* table_type);

/**
 * @brief Get number of partitions on USB drive
 * @param count Pointer to store partition count
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_get_partition_count(uint8_t* count);

/**
 * @brief Get partition information
 * @param partition_num Partition number (0-3)
 * @param info Pointer to store partition information
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_get_partition_info(uint8_t partition_num, partition_info_t* info);

/**
 * @brief Delete all partitions (zero MBR)
 * WARNING: This is a DESTRUCTIVE operation! All data will be lost!
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_delete_all_partitions(void);
