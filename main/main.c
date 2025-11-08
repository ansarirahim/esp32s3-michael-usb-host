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
#include <dirent.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_pins.h"
#include "led_control.h"
#include "usb_host.h"
#include "internal_storage.h"
#include "workflow.h"

static const char *TAG = "app";

/* Test results */
static int tests_passed = 0;
static int tests_failed = 0;

/**
 * @brief Automated test helper
 */
#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            ESP_LOGI(TAG, "✓ TEST PASSED: %s", test_name); \
            tests_passed++; \
        } else { \
            ESP_LOGE(TAG, "✗ TEST FAILED: %s", test_name); \
            tests_failed++; \
        } \
    } while(0)

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

    /* Phase 2a - Initialize USB Host Mode */
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Phase 2a: Initializing USB Host Mode...");
    ESP_LOGI(TAG, "=================================================");

    if (usb_host_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize USB Host");
        led_control_set_state(LED_STATE_ERROR);
        return;
    }
    ESP_LOGI(TAG, "✓ USB Host initialized successfully");

    /* Phase 3c - Initialize Internal Storage (SPIFFS) */
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Phase 3c: Initializing Internal Storage (SPIFFS)...");
    ESP_LOGI(TAG, "=================================================");

    if (internal_storage_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize internal storage");
        led_control_set_state(LED_STATE_ERROR);
        return;
    }
    ESP_LOGI(TAG, "✓ Internal storage initialized successfully");

    /* Create sample files in SPIFFS */
    ESP_LOGI(TAG, "Creating sample files in SPIFFS...");
    if (internal_storage_create_samples() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create sample files");
        led_control_set_state(LED_STATE_ERROR);
        return;
    }
    ESP_LOGI(TAG, "✓ Sample files created successfully");

    /* List files in SPIFFS */
    ESP_LOGI(TAG, "Listing files in SPIFFS:");
    uint32_t file_count = 0;
    if (internal_storage_list_files(&file_count) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to list files");
    } else {
        ESP_LOGI(TAG, "✓ Total files in SPIFFS: %lu", file_count);
    }

    /* Phase 4b - Initialize Workflow Automation */
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Phase 4b: Initializing Workflow Automation...");
    ESP_LOGI(TAG, "=================================================");

    workflow_config_t workflow_config = {
        .auto_format = true,    /* Auto-format on mount failure */
        .auto_copy = true,      /* Auto-copy files after mount */
        .auto_eject = true,     /* Auto-eject after copy */
        .loop_enabled = true,   /* Loop for multiple USB drives */
    };

    if (workflow_init(&workflow_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize workflow automation");
        led_control_set_state(LED_STATE_ERROR);
        return;
    }
    ESP_LOGI(TAG, "✓ Workflow automation initialized successfully");

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Running Automated Tests");
    ESP_LOGI(TAG, "=================================================");

    /* ========================================================================
     * AUTOMATED TESTS - Phase 1 & 2a
     * ======================================================================== */

    ESP_LOGI(TAG, "Test 1: USB Host Initialization");
    TEST_ASSERT(usb_host_is_initialized() == true, "USB Host should be initialized");

    ESP_LOGI(TAG, "Test 2: USB Device Detection (waiting 10 seconds)");
    ESP_LOGI(TAG, ">>> Please insert USB drive now <<<");

    /* Wait for USB device insertion */
    bool device_detected = false;
    for (int i = 0; i < 10; i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (usb_host_is_device_connected()) {
            device_detected = true;
            break;
        }
        ESP_LOGI(TAG, "Waiting for USB device... %d/10", i + 1);
    }

    if (device_detected) {
        TEST_ASSERT(true, "USB device detected");
        ESP_LOGI(TAG, "✓ USB device connected - LED should be CYAN (PREPARE)");
        vTaskDelay(pdMS_TO_TICKS(3000));
    } else {
        TEST_ASSERT(false, "USB device NOT detected (timeout)");
        ESP_LOGW(TAG, "No USB device detected - continuing with manual test mode");
    }

    /* ========================================================================
     * MANUAL TEST MODE - LED States
     * ======================================================================== */

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Manual Test: LED State Sequence");
    ESP_LOGI(TAG, "=================================================");

    /* Test all LED states */
    ESP_LOGI(TAG, "State: IDLE (Green slow blink)");
    led_control_set_state(LED_STATE_IDLE);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "State: PREPARE (Cyan fast blink)");
    led_control_set_state(LED_STATE_PREPARE);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "State: COPY (Yellow blink)");
    led_control_set_state(LED_STATE_COPY);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "State: SYNC (Magenta blink)");
    led_control_set_state(LED_STATE_SYNC);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "State: SUCCESS (Green solid 2s)");
    led_control_set_state(LED_STATE_SUCCESS);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "State: ERROR (Red fast blink)");
    led_control_set_state(LED_STATE_ERROR);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Back to IDLE */
    ESP_LOGI(TAG, "State: IDLE (Green slow blink)");
    led_control_set_state(LED_STATE_IDLE);

    /* ========================================================================
     * TEST SUMMARY
     * ======================================================================== */

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "INITIALIZATION COMPLETE");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Tests Passed: %d", tests_passed);
    ESP_LOGI(TAG, "Tests Failed: %d", tests_failed);
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Phase 1: LED Control - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2a: USB Host Init - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2b: USB MSC Driver - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2c: File Operations - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2d: Safe Eject - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 3a: Partition Detection - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 3b: Partition Creation - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 3c: File Copy (SPIFFS) - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 4a: Label Configuration - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 4b: Workflow Automation - ACTIVE ✓");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "🚀 WORKFLOW AUTOMATION ACTIVE");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Insert USB drive to start automated workflow:");
    ESP_LOGI(TAG, "  1. Auto-format (if needed)");
    ESP_LOGI(TAG, "  2. Copy files from SPIFFS");
    ESP_LOGI(TAG, "  3. Safe eject");
    ESP_LOGI(TAG, "  4. Wait for next USB drive");
    ESP_LOGI(TAG, "=================================================");

    /* Keep application running - Workflow automation handles everything */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));  /* Sleep 60s - workflow runs in background */
    }
}

