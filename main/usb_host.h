/**
 * @file usb_host.h
 * @brief USB Host Mode Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 3.0.0
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
