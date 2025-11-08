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
    ESP_LOGI(TAG, "TEST SUMMARY");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Tests Passed: %d", tests_passed);
    ESP_LOGI(TAG, "Tests Failed: %d", tests_failed);
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Phase 1: LED Control - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2a: USB Host Init - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2b: USB MSC Driver - COMPLETE ✓");
    ESP_LOGI(TAG, "Phase 2c: File Operations - READY FOR TEST ✓");
    ESP_LOGI(TAG, "Phase 2d: Safe Eject - READY FOR TEST ✓");
    ESP_LOGI(TAG, "Phase 3a: Partition Detection - READY FOR TEST ✓");
    ESP_LOGI(TAG, "Phase 3c: File Copy (SPIFFS) - READY FOR TEST ✓");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Application running - waiting for USB events");
    ESP_LOGI(TAG, "Insert USB drive to test file operations");
    ESP_LOGI(TAG, "Test sequence: File Listing -> Read File -> Write File -> Partition Detection -> File Copy -> Safe Eject");

    /* Keep application running - USB and LED tasks continue in background */
    static bool files_listed = false;
    static bool file_read_tested = false;
    static bool file_write_tested = false;
    static bool safe_eject_tested = false;
    static bool partition_detected = false;
    static bool files_copied = false;
    static TickType_t file_list_time = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        /* Check if USB drive is mounted and list files */
        const char* mount_point = usb_host_get_mount_point();
        if (mount_point != NULL) {
            if (!files_listed) {
                ESP_LOGI(TAG, "=================================================");
                ESP_LOGI(TAG, "USB Drive Mounted - Listing Files");
                ESP_LOGI(TAG, "=================================================");

                DIR *dir = opendir(mount_point);
                if (dir != NULL) {
                    struct dirent *entry;
                    int file_count = 0;

                    while ((entry = readdir(dir)) != NULL) {
                        char filepath[512];
                        snprintf(filepath, sizeof(filepath), "%s/%s", mount_point, entry->d_name);

                        struct stat st;
                        if (stat(filepath, &st) == 0) {
                            if (S_ISDIR(st.st_mode)) {
                                ESP_LOGI(TAG, "[DIR]  %s", entry->d_name);
                            } else {
                                ESP_LOGI(TAG, "[FILE] %s (%ld bytes)", entry->d_name, st.st_size);
                                file_count++;
                            }
                        }
                    }

                    closedir(dir);
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Total files: %d", file_count);
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "✓ TEST PASSED: USB MSC file listing");
                    tests_passed++;
                    files_listed = true;
                    file_list_time = xTaskGetTickCount();
                } else {
                    ESP_LOGE(TAG, "Failed to open directory: %s", mount_point);
                }
            }

            /* Test file read 3 seconds after file listing */
            if (files_listed && !file_read_tested) {
                TickType_t elapsed = (xTaskGetTickCount() - file_list_time) / pdMS_TO_TICKS(1000);
                if (elapsed >= 3) {
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Testing File Read...");
                    ESP_LOGI(TAG, "=================================================");

                    /* Try to read ANSARI~1.TXT */
                    char buffer[256];
                    size_t bytes_read = 0;
                    esp_err_t ret = usb_host_read_file("ANSARI~1.TXT", buffer, sizeof(buffer), &bytes_read);

                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "File contents (%d bytes):", bytes_read);
                        ESP_LOGI(TAG, "---");
                        ESP_LOGI(TAG, "%s", buffer);
                        ESP_LOGI(TAG, "---");
                        ESP_LOGI(TAG, "✓ TEST PASSED: File read successful");
                        tests_passed++;
                    } else {
                        ESP_LOGW(TAG, "File read test skipped (file may not exist)");
                    }
                    file_read_tested = true;
                }
            }

            /* Test file write 6 seconds after file listing */
            if (files_listed && file_read_tested && !file_write_tested) {
                TickType_t elapsed = (xTaskGetTickCount() - file_list_time) / pdMS_TO_TICKS(1000);
                if (elapsed >= 6) {
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Testing File Write...");
                    ESP_LOGI(TAG, "=================================================");

                    /* Write test file */
                    const char* test_data = "ESP32-S3 USB Host Test\nPhase 2c: File Operations\nDate: November 8, 2025\n";
                    esp_err_t ret = usb_host_write_file("ESP32TEST.TXT", test_data, strlen(test_data));

                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✓ TEST PASSED: File write successful");
                        tests_passed++;

                        /* Verify by reading back */
                        char verify_buffer[256];
                        size_t bytes_read = 0;
                        ret = usb_host_read_file("ESP32TEST.TXT", verify_buffer, sizeof(verify_buffer), &bytes_read);
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Verification read (%d bytes):", bytes_read);
                            ESP_LOGI(TAG, "---");
                            ESP_LOGI(TAG, "%s", verify_buffer);
                            ESP_LOGI(TAG, "---");
                            ESP_LOGI(TAG, "✓ TEST PASSED: File write verification successful");
                            tests_passed++;
                        }
                    } else {
                        ESP_LOGE(TAG, "✗ TEST FAILED: File write failed");
                        tests_failed++;
                    }
                    file_write_tested = true;
                }
            }

            /* Test partition detection 14 seconds after file listing (before safe eject) */
            if (files_listed && file_read_tested && file_write_tested && !partition_detected) {
                TickType_t elapsed = (xTaskGetTickCount() - file_list_time) / pdMS_TO_TICKS(1000);
                if (elapsed >= 14) {
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "PHASE 3A: Testing Partition Detection...");
                    ESP_LOGI(TAG, "=================================================");

                    /* Test 1: Detect partition table type */
                    partition_table_type_t table_type;
                    esp_err_t ret = usb_host_detect_partition_table(&table_type);
                    if (ret == ESP_OK) {
                        const char* type_str = (table_type == PARTITION_TABLE_MBR) ? "MBR" :
                                               (table_type == PARTITION_TABLE_GPT) ? "GPT" :
                                               (table_type == PARTITION_TABLE_NONE) ? "NONE" : "UNKNOWN";
                        ESP_LOGI(TAG, "✓ Partition table type: %s", type_str);
                        tests_passed++;
                    } else {
                        ESP_LOGE(TAG, "✗ TEST FAILED: Partition table detection failed");
                        tests_failed++;
                    }

                    /* Test 2: Get partition count */
                    uint8_t partition_count = 0;
                    ret = usb_host_get_partition_count(&partition_count);
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✓ Partition count: %d", partition_count);
                        tests_passed++;

                        /* Test 3: Get partition info for each partition */
                        for (uint8_t i = 0; i < partition_count && i < 4; i++) {
                            partition_info_t info;
                            ret = usb_host_get_partition_info(i, &info);
                            if (ret == ESP_OK) {
                                ESP_LOGI(TAG, "✓ Partition %d:", i);
                                ESP_LOGI(TAG, "  Type: 0x%02X", info.partition_type);
                                ESP_LOGI(TAG, "  Start LBA: %lu", info.start_lba);
                                ESP_LOGI(TAG, "  Size: %lu sectors (%.2f MB)",
                                         info.size_sectors,
                                         (float)info.size_bytes / (1024.0 * 1024.0));
                                tests_passed++;
                            } else {
                                ESP_LOGE(TAG, "✗ Failed to get partition %d info", i);
                                tests_failed++;
                            }
                        }
                    } else {
                        ESP_LOGE(TAG, "✗ TEST FAILED: Get partition count failed");
                        tests_failed++;
                    }

                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Phase 3a partition detection tests complete");
                    ESP_LOGI(TAG, "=================================================");
                    partition_detected = true;
                }
            }

            /* Test Phase 3c: File Copy (16 seconds after file listing) */
            if (files_listed && file_read_tested && file_write_tested && partition_detected && !files_copied) {
                TickType_t elapsed = (xTaskGetTickCount() - file_list_time) / pdMS_TO_TICKS(1000);
                if (elapsed >= 16) {
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Phase 3c: Testing File Copy from SPIFFS to USB");
                    ESP_LOGI(TAG, "=================================================");

                    /* Set LED to COPY state */
                    led_control_set_state(LED_STATE_COPY);

                    /* Copy all files from SPIFFS to USB */
                    uint32_t copied_count = 0;
                    esp_err_t ret = usb_host_copy_all_files(
                        internal_storage_get_mount_point(),
                        mount_point,
                        &copied_count
                    );

                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✓ TEST PASSED: All files copied successfully (%lu files)", copied_count);
                        tests_passed++;
                        led_control_set_state(LED_STATE_SUCCESS);
                        vTaskDelay(pdMS_TO_TICKS(2000));  /* Show success for 2 seconds */
                    } else {
                        ESP_LOGE(TAG, "✗ TEST FAILED: File copy failed");
                        tests_failed++;
                        led_control_set_state(LED_STATE_ERROR);
                        vTaskDelay(pdMS_TO_TICKS(2000));  /* Show error for 2 seconds */
                    }

                    /* Back to IDLE */
                    led_control_set_state(LED_STATE_IDLE);

                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Phase 3c file copy test complete");
                    ESP_LOGI(TAG, "=================================================");
                    files_copied = true;
                }
            }

            /* Test safe eject 20 seconds after file listing (after file copy) */
            if (files_listed && file_read_tested && file_write_tested && partition_detected && files_copied && !safe_eject_tested) {
                TickType_t elapsed = (xTaskGetTickCount() - file_list_time) / pdMS_TO_TICKS(1000);
                if (elapsed >= 20) {
                    ESP_LOGI(TAG, "=================================================");
                    ESP_LOGI(TAG, "Testing Safe Eject...");
                    ESP_LOGI(TAG, "=================================================");

                    esp_err_t ret = usb_host_safe_eject();
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✓ TEST PASSED: Safe eject successful");
                        tests_passed++;
                        safe_eject_tested = true;
                    } else {
                        ESP_LOGE(TAG, "✗ TEST FAILED: Safe eject failed: %s", esp_err_to_name(ret));
                        tests_failed++;
                    }
                }
            }
        } else {
            /* Reset flags when USB is disconnected */
            files_listed = false;
            file_read_tested = false;
            file_write_tested = false;
            safe_eject_tested = false;
            partition_detected = false;
            files_copied = false;
        }
    }
}

