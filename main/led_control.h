/**
 * @file led_control.h
 * @brief WS2812B RGB LED Control Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Implements RMT-based WS2812B RGB LED control with state machine
 */

#pragma once

#include "esp_err.h"

/**
 * @brief LED States
 */
typedef enum {
    /* USB Host Mode States */
    LED_STATE_IDLE = 0,              /**< Green slow blink - Waiting for USB (Host mode) */
    LED_STATE_PREPARE = 1,           /**< Cyan fast blink - Preparing USB drive */
    LED_STATE_COPY = 2,              /**< Yellow blink - Copying files */
    LED_STATE_SYNC = 3,              /**< Magenta blink - Syncing filesystem */
    LED_STATE_SUCCESS = 4,           /**< Green solid 2s - Operation complete */
    LED_STATE_ERROR = 5,             /**< Red fast blink - Error occurred */

    /* USB Device Mode States (Phase 5) */
    LED_STATE_DEVICE_IDLE = 6,       /**< Blue slow blink - Waiting for PC (Device mode) */
    LED_STATE_DEVICE_MOUNTED = 7,    /**< Blue solid - PC connected */
    LED_STATE_DEVICE_ACTIVE = 8,     /**< Magenta blink - PC reading/writing */

    /* Mode Switching States (Phase 5) */
    LED_STATE_MODE_SWITCHING = 9,    /**< Orange fast blink - Mode switch in progress */
} led_state_t;

/**
 * @brief Initialize LED control
 * @param gpio_pin GPIO pin number for WS2812B LED
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t led_control_init(int gpio_pin);

/**
 * @brief Set LED state
 * @param state LED state to set
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t led_control_set_state(led_state_t state);

/**
 * @brief Get current LED state
 * @return Current LED state
 */
led_state_t led_control_get_state(void);

/**
 * @brief Deinitialize LED control
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t led_control_deinit(void);

