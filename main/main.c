/**
 * @file main.c
 * @brief ESP32-S3 USB Host Automator - Main Application
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Project: USB Host Mode Automation for Michael Steinmann
 * 
 * Implements automated USB host workflow:
 * Step 1: Prepare USB drive (delete partition + format)
 * Step 2: Copy files + sync + eject
 * Step 3: Wait and repeat
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_pins.h"
#include "led_control.h"

static const char *TAG = "app";

/**
 * @brief Application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "ESP32-S3 USB Host Automator");
    ESP_LOGI(TAG, "Project: USB Host Mode Automation");
    ESP_LOGI(TAG, "Author: Abdul Raheem Ansari");
    ESP_LOGI(TAG, "Date: November 2025");
    ESP_LOGI(TAG, "=================================================");

    /* Initialize board pins and detect LED GPIO */
    ESP_LOGI(TAG, "Initializing board pins...");
    if (board_pins_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize board pins");
        return;
    }

    ESP_LOGI(TAG, "Board: %s", board_pins_get_board_name());
    ESP_LOGI(TAG, "LED GPIO: %d", board_pins_get_led_gpio());

    /* Phase 1 - Initialize LED Control */
    ESP_LOGI(TAG, "Phase 1: Initializing LED Control...");
    if (led_control_init(board_pins_get_led_gpio()) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED control");
        return;
    }
    ESP_LOGI(TAG, "✓ LED Control initialized successfully");

    /* TODO: Phase 2 - Initialize USB Host Mode */
    ESP_LOGI(TAG, "TODO: Initialize USB Host Mode (Phase 2)");

    /* TODO: Phase 3 - Initialize USB Host Automator */
    ESP_LOGI(TAG, "TODO: Initialize USB Host Automator (Phase 3)");

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Application ready - Testing LED states");
    ESP_LOGI(TAG, "=================================================");

    /* Test LED states (Phase 1 demonstration) */
    ESP_LOGI(TAG, "Starting LED state test sequence...");

    /* Start with IDLE state (default) */
    ESP_LOGI(TAG, "State: IDLE (Green slow blink)");
    vTaskDelay(pdMS_TO_TICKS(5000));

    /* Test PREPARE state */
    ESP_LOGI(TAG, "State: PREPARE (Cyan fast blink)");
    led_control_set_state(LED_STATE_PREPARE);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Test COPY state */
    ESP_LOGI(TAG, "State: COPY (Yellow blink)");
    led_control_set_state(LED_STATE_COPY);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Test SYNC state */
    ESP_LOGI(TAG, "State: SYNC (Magenta blink)");
    led_control_set_state(LED_STATE_SYNC);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Test SUCCESS state */
    ESP_LOGI(TAG, "State: SUCCESS (Green solid 2s)");
    led_control_set_state(LED_STATE_SUCCESS);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Test ERROR state */
    ESP_LOGI(TAG, "State: ERROR (Red fast blink)");
    led_control_set_state(LED_STATE_ERROR);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Back to IDLE */
    ESP_LOGI(TAG, "State: IDLE (Green slow blink)");
    led_control_set_state(LED_STATE_IDLE);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "LED state test complete - staying in IDLE state");
    ESP_LOGI(TAG, "Phase 1: LED Control - COMPLETE ✓");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Application running - LED will continue blinking in IDLE state");

    /* Keep application running - LED task continues in background */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        /* Application is idle - LED animation task is running */
    }
}

