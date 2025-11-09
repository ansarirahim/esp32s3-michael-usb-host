/**
 * @file usb_device.h
 * @brief USB Device Mode Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize USB Device Mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_device_init(void);

/**
 * @brief Deinitialize USB Device Mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_device_deinit(void);

/**
 * @brief Check if USB Device is connected to PC
 * @return true if connected, false otherwise
 */
bool usb_device_is_connected(void);

