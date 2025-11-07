/**
 * @file usb_host.h
 * @brief USB Host Mode Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 5.0.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

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
