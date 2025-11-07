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

esp_err_t usb_device_init(void);
esp_err_t usb_device_deinit(void);

