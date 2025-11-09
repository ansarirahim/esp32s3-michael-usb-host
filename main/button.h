/**
 * @file button.h
 * @brief Button Handler with Triple-Press Detection
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Implements GPIO interrupt-based button handler with:
 * - 50ms debounce timer
 * - Triple-press detection (3 presses within 2 seconds)
 * - Callback on successful triple-press
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Button event callback function type
 * @param user_data User data passed during initialization
 */
typedef void (*button_event_callback_t)(void* user_data);

/**
 * @brief Button configuration structure
 */
typedef struct {
    int gpio_num;                           /**< GPIO pin number for button */
    uint32_t debounce_ms;                   /**< Debounce time in milliseconds (default: 50ms) */
    uint32_t triple_press_window_ms;        /**< Triple-press detection window in milliseconds (default: 2000ms) */
    uint32_t min_inter_press_ms;            /**< Minimum time between presses in milliseconds (default: 150ms) */
    button_event_callback_t callback;       /**< Callback function for triple-press event */
    void* user_data;                        /**< User data passed to callback */
} button_config_t;

/**
 * @brief Initialize button handler
 * @param config Button configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_init(const button_config_t* config);

/**
 * @brief Deinitialize button handler
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_deinit(void);

/**
 * @brief Enable button handler
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_enable(void);

/**
 * @brief Disable button handler
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_disable(void);

/**
 * @brief Check if button handler is enabled
 * @return true if enabled, false otherwise
 */
bool button_is_enabled(void);

/**
 * @brief Get button press count (for debugging)
 * @return Number of button presses detected
 */
uint32_t button_get_press_count(void);

/**
 * @brief Reset button press count
 */
void button_reset_press_count(void);

