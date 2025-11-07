/**
 * @file usb_host_automator.h
 * @brief USB Host Automator FSM Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Implements Michael's USB Host workflow:
 * Step 1: Prepare USB drive (delete partition + format)
 * Step 2: Copy files + sync + eject
 * Step 3: Wait and repeat
 */

#pragma once

#include "esp_err.h"

esp_err_t usb_host_automator_init(void);
esp_err_t usb_host_automator_deinit(void);

