/**
 * @file usb_mode_manager.h
 * @brief USB Mode Manager - Runtime Mode Switching Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Manages USB mode switching between Host and Device modes at runtime.
 * Handles USB stack deinitialization and reinitialization.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief USB Mode enumeration
 */
typedef enum {
    USB_MODE_HOST = 0,      /**< USB Host mode (ESP32 controls USB flash drives) */
    USB_MODE_DEVICE = 1,    /**< USB Device mode (ESP32 appears as USB device to PC) */
} usb_mode_t;

/**
 * @brief USB Mode Manager statistics
 */
typedef struct {
    usb_mode_t current_mode;        /**< Current USB mode */
    uint32_t mode_switches;         /**< Total number of mode switches */
    uint32_t switch_failures;       /**< Number of failed mode switches */
    uint64_t uptime_ms;             /**< System uptime in milliseconds */
    uint64_t last_switch_time_ms;   /**< Time of last mode switch in milliseconds */
} usb_mode_stats_t;

/**
 * @brief Initialize USB Mode Manager
 * @param initial_mode Initial USB mode (HOST or DEVICE)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_mode_manager_init(usb_mode_t initial_mode);

/**
 * @brief Deinitialize USB Mode Manager
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_mode_manager_deinit(void);

/**
 * @brief Switch USB mode
 * @param new_mode New USB mode (HOST or DEVICE)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_mode_manager_switch(usb_mode_t new_mode);

/**
 * @brief Toggle USB mode (HOST <-> DEVICE)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_mode_manager_toggle(void);

/**
 * @brief Get current USB mode
 * @return Current USB mode (HOST or DEVICE)
 */
usb_mode_t usb_mode_manager_get_mode(void);

/**
 * @brief Get USB mode as string
 * @param mode USB mode
 * @return Mode name ("HOST" or "DEVICE")
 */
const char* usb_mode_manager_get_mode_name(usb_mode_t mode);

/**
 * @brief Check if mode switching is allowed
 * @return true if allowed, false otherwise
 */
bool usb_mode_manager_can_switch(void);

/**
 * @brief Get mode manager statistics
 * @param stats Pointer to statistics structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_mode_manager_get_stats(usb_mode_stats_t* stats);

/**
 * @brief Check if USB Mode Manager is initialized
 * @return true if initialized, false otherwise
 */
bool usb_mode_manager_is_initialized(void);

