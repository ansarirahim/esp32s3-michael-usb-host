/**
 * @file usb_host.h
 * @brief USB Host Mode Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t usb_host_init(void);
esp_err_t usb_host_deinit(void);
bool usb_host_is_device_connected(void);

