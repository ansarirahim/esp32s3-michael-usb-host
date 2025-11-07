/**
 * @file board_pins.h
 * @brief Hardware Pin Definitions for ESP32-S3 DevKitC-1
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Board: ESP32-S3 DevKitC-1 (N8R8 / N16R8)
 * USB PHY: Native (GPIO20=D+, GPIO19=D-)
 * LED: WS2812B RGB LED (GPIO38 or GPIO48 depending on board)
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

/* ============================================================================
 * USB PINS (Native PHY)
 * ============================================================================ */

/** @brief USB Data+ line (GPIO20) */
#define PIN_USB_DP        20

/** @brief USB Data- line (GPIO19) */
#define PIN_USB_DM        19

/* ============================================================================
 * LED PINS (WS2812B RGB LED)
 * ============================================================================ */

/** @brief RGB LED GPIO pin (auto-detected: GPIO38 for COM14, GPIO48 for COM11) */
#define PIN_LED_RGB       -1  /* Will be auto-detected at runtime */

/** @brief RGB LED RMT channel */
#define LED_RMT_CHANNEL   RMT_CHANNEL_0

/** @brief RGB LED RMT resolution (10 MHz) */
#define LED_RMT_RESOLUTION_HZ  (10 * 1000 * 1000)

/* ============================================================================
 * BOOT/MODE SELECTION
 * ============================================================================ */

/** @brief GPIO0 - BOOT button (reserved for future use) */
#define PIN_BOOT1         0

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/**
 * @brief Initialize board pins and auto-detect LED GPIO
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t board_pins_init(void);

/**
 * @brief Get the detected LED GPIO pin
 * @return GPIO pin number (38 or 48), or -1 if not detected
 */
int board_pins_get_led_gpio(void);

/**
 * @brief Get board identifier string
 * @return Board name (e.g., "Espressif DevKitC-1-N8R8")
 */
const char* board_pins_get_board_name(void);

