/*
 * SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <cctype>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "usb/cdc_acm_host.h"
#include "usb/vcp_ch34x.hpp"
#include "usb/vcp_cp210x.hpp"
#include "usb/vcp_ftdi.hpp"
#include "usb/vcp.hpp"
#include "usb/usb_host.h"
#include "lpc21isp_esp32.h"
#include "rgb_led.h"
#include "esp32_serial_adapter.h"

// Forward declarations for sync flag management
extern "C" {
void lpc21isp_esp32_set_synchronized(void);
void lpc21isp_esp32_reset_sync_flag(void);
bool lpc21isp_esp32_is_synchronized(void);
void ClearSerialPortBuffers(ISP_ENVIRONMENT *IspEnvironment);
// SendComPort is already declared in lpc21isp.h (included via lpc21isp_esp32.h)
}

// Forward declarations for FatFS functions
extern "C" {
esp_err_t fatfs_init(void);
void fatfs_list_files(void);
esp_err_t fatfs_write_file(const char *filename, const char *content);
esp_err_t fatfs_write_binary_file(const char *filename, const uint8_t *data, size_t data_len);
bool fatfs_file_exists(const char *filename);
esp_err_t fatfs_read_binary_file(const char *filename, uint8_t **buffer, size_t *file_size);
// Forward declaration for MSC device mode
void msc_device_init(void);
}

using namespace esp_usb;

// Change these values to match your needs
//#define LPC21ISP_BAUDRATE     (38400)
#define LPC21ISP_BAUDRATE     (115200)
#define LPC21ISP_STOP_BITS    (0)      // 0: 1 stopbit, 1: 1.5 stopbits, 2: 2 stopbits
#define LPC21ISP_PARITY       (0)      // 0: None, 1: Odd, 2: Even, 3: Mark, 4: Space
#define LPC21ISP_DATA_BITS    (8)

namespace {
static const char *TAG = "VCP_LPC21ISP";
static SemaphoreHandle_t device_disconnected_sem;
static volatile bool bootloader_synchronized = false;  // Flag to indicate bootloader sync
static volatile bool programming_in_progress = false;  // Flag to disable logging during programming
static volatile bool device_disconnected = false;  // Flag to indicate device disconnection

// GPIO0 (BOOT0 button) configuration
#define BOOT0_BUTTON_GPIO GPIO_NUM_0
#define BUTTON_PRESS_TIME_MS 2000  // 2 seconds

// Mode definitions
typedef enum {
    MODE_CDC_HOST = 0,
    MODE_MSC_DEVICE = 1
} device_mode_t;

// NVRAM configuration
#define NVS_NAMESPACE "device_config"
#define NVS_KEY_MODE "mode"

// Queue for deferred processing of received data
#define RX_DATA_QUEUE_SIZE 10
#define MAX_RX_DATA_SIZE 512
typedef struct {
    uint8_t data[MAX_RX_DATA_SIZE];
    size_t data_len;
} rx_data_item_t;
static QueueHandle_t rx_data_queue = NULL;
static TaskHandle_t rx_processing_task_handle = NULL;

// Current device mode (needs to be accessible to button task)
static device_mode_t current_mode = MODE_CDC_HOST;

// Forward declarations
static esp_err_t nvram_write_mode(device_mode_t mode);
static esp_err_t nvram_read_mode(device_mode_t *mode);

/**
 * @brief Task to process received data from queue
 * This runs at lower priority to avoid crashes during programming
 * 
 * @param arg Unused task argument
 */
static void rx_processing_task(void *arg)
{
    rx_data_item_t item;
    
    while (1) {
        // Wait for data from queue (blocking)
        if (xQueueReceive(rx_data_queue, &item, portMAX_DELAY) == pdTRUE) {
            // Skip ALL processing during programming - NxpDownload corrupts memory including heap and logging
            // This includes skipping logging, "Synchronized" detection, etc.
            // Just drop the data to avoid any heap/logging operations
            if (programming_in_progress) {
                // Programming in progress - drop data to avoid crashes
                continue;
            }
            
            // Process the received data only when NOT programming
            if (item.data_len > 0) {
                // Log the data as a string (for text data)
                ESP_LOGI(TAG, "Received %d bytes: %.*s", item.data_len, item.data_len, item.data);
                
                // Check if we received "Synchronized" - bootloader is activated
                // Use stack-allocated buffer instead of malloc to avoid heap corruption issues
                // Limit to MAX_RX_DATA_SIZE to avoid stack overflow
                size_t check_len = (item.data_len < MAX_RX_DATA_SIZE) ? item.data_len : MAX_RX_DATA_SIZE;
                char str[MAX_RX_DATA_SIZE + 1];
                memcpy(str, item.data, check_len);
                str[check_len] = '\0';
                
                // Case-insensitive check for "Synchronized"
                char *found = strstr(str, "Synchronized");
                if (found == NULL) {
                    // Try lowercase - convert in place
                    for (size_t i = 0; i < check_len; i++) {
                        str[i] = tolower((unsigned char)str[i]);
                    }
                    found = strstr(str, "synchronized");
                }
                if (found != NULL) {
                    ESP_LOGI(TAG, "Bootloader activated - received 'Synchronized'");
                    bootloader_synchronized = true;  // Set local flag
                    lpc21isp_esp32_set_synchronized();  // Set flag in wrapper so sync function can see it
                    rgb_led_set_state(LED_STATE_SYNCHRONIZED);
                }
                
                // Also log as hex for debugging binary data
                char hex_str[256];
                size_t hex_len = (item.data_len * 3) + 1; // Each byte takes 3 chars (XX ) + null terminator
                if (hex_len <= sizeof(hex_str)) {
                    char *p = hex_str;
                    for (size_t i = 0; i < item.data_len && i < 85; i++) { // Limit to 85 bytes to fit in log line
                        p += sprintf(p, "%02X ", item.data[i]);
                    }
                    ESP_LOGI(TAG, "Hex: %s", hex_str);
                }
            }
        }
    }
}

/**
 * @brief Task to monitor BOOT0 button (GPIO0)
 * Detects when button is pressed for 2 seconds and toggles the mode
 */
static void boot0_button_task(void *arg)
{
    bool button_pressed = false;
    int press_duration_ms = 0;
    bool mode_toggled = false;
    
    while (1) {
        // Read GPIO0 state (button is active low, so pressed = 0)
        int level = gpio_get_level(BOOT0_BUTTON_GPIO);
        bool is_pressed = (level == 0);
        
        if (is_pressed) {
            if (!button_pressed) {
                // Button just pressed - start timing
                button_pressed = true;
                press_duration_ms = 0;
                mode_toggled = false;
            } else {
                // Button still pressed - increment timer
                press_duration_ms += 100; // Check every 100ms
                
                // Check if pressed for 2 seconds
                if (press_duration_ms >= BUTTON_PRESS_TIME_MS && !mode_toggled) {
                    // Toggle mode
                    device_mode_t old_mode = current_mode;
                    device_mode_t new_mode = (current_mode == MODE_CDC_HOST) ? MODE_MSC_DEVICE : MODE_CDC_HOST;
                    
                    ESP_LOGI(TAG, "BOOT0 button pressed for 2 seconds - toggling mode");
                    ESP_LOGI(TAG, "Current mode: %s (value: %d) -> New mode: %s (value: %d)", 
                            (old_mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE", old_mode,
                            (new_mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE", new_mode);
                    
                    esp_err_t err = nvram_write_mode(new_mode);
                    if (err == ESP_OK) {
                        current_mode = new_mode;
                        ESP_LOGI(TAG, "Mode successfully saved to NVRAM: %s (value: %d)", 
                                (current_mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE", current_mode);
                        ESP_LOGI(TAG, "Resetting MCU to apply new mode...");
                        
                        // Give some time for log messages to be sent
                        vTaskDelay(pdMS_TO_TICKS(200));
                        
                        // Reset the MCU to apply the new mode
                        esp_restart();
                    } else {
                        ESP_LOGE(TAG, "Failed to save mode to NVRAM: %s", esp_err_to_name(err));
                    }
                    
                    mode_toggled = true; // Prevent multiple toggles while button is held
                }
            }
        } else {
            // Button released - reset state
            if (button_pressed) {
                button_pressed = false;
                press_duration_ms = 0;
                mode_toggled = false;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}

/**
 * @brief Data received callback
 * 
 * This callback runs at high priority (from USB interrupt context).
 * We just enqueue the data to be processed later by a lower-priority task.
 * This prevents crashes when NxpDownload is corrupting memory.
 */
static bool handle_rx(const uint8_t *data, size_t data_len, void *arg)
{
    // Always add data to the serial adapter buffer (for ReceiveComPortBlock)
    // This is critical for NxpDownload to receive bootloader responses
    if (data_len > 0) {
        esp32_serial_adapter_add_rx_data(data, data_len);
    }
    
    // Only enqueue to logging queue when NOT programming
    // During programming, NxpDownload reads directly from the serial buffer above
    if (programming_in_progress) {
        return true;  // Skip logging queue during programming
    }
    
    if (data_len > 0 && rx_data_queue != NULL) {
        // Create queue item for logging/debugging
        rx_data_item_t item;
        size_t copy_len = (data_len < MAX_RX_DATA_SIZE) ? data_len : MAX_RX_DATA_SIZE;
        memcpy(item.data, data, copy_len);
        item.data_len = copy_len;
        
        // Send to queue (non-blocking - drop if queue is full to avoid blocking interrupt)
        if (xQueueSend(rx_data_queue, &item, 0) != pdTRUE) {
            // Queue full - data will be dropped
            // This is acceptable as we're just logging/debugging
        }
    }
    return true;
}

/**
 * @brief Device event callback
 *
 * Apart from handling device disconnection it doesn't do anything useful
 *
 * @param[in] event    Device event type and data
 * @param[in] user_ctx Argument we passed to the device open function
 */
static void handle_event(const cdc_acm_host_dev_event_data_t *event, void *user_ctx)
{
    switch (event->type) {
    case CDC_ACM_HOST_ERROR:
        ESP_LOGE(TAG, "CDC-ACM error has occurred, err_no = %d", event->data.error);
        // Errors often indicate device disconnection, set flag to allow graceful exit
        device_disconnected = true;
        break;
    case CDC_ACM_HOST_DEVICE_DISCONNECTED:
        ESP_LOGI(TAG, "Device suddenly disconnected");
        device_disconnected = true;  // Set flag to signal disconnection
        rgb_led_set_state(LED_STATE_IDLE); // Reset to idle when device disconnects
        xSemaphoreGive(device_disconnected_sem);
        break;
    case CDC_ACM_HOST_SERIAL_STATE:
        ESP_LOGI(TAG, "Serial state notif 0x%04X", event->data.serial_state.val);
        break;
    case CDC_ACM_HOST_NETWORK_CONNECTION:
    default: break;
    }
}

/**
 * @brief Write device mode to NVRAM
 * 
 * @param mode Mode to write
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t nvram_write_mode(device_mode_t mode)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u8(nvs_handle, NVS_KEY_MODE, (uint8_t)mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing mode to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Mode saved to NVRAM: %s (value: %d)", 
            (mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE", mode);
    return ESP_OK;
}

/**
 * @brief Read device mode from NVRAM
 * 
 * @param mode Pointer to store the read mode
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t nvram_read_mode(device_mode_t *mode)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Try to open with READWRITE first (will create namespace if it doesn't exist)
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        // If that fails, try READONLY
        err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
            return err;
        }
    }

    uint8_t mode_value;
    err = nvs_get_u8(nvs_handle, NVS_KEY_MODE, &mode_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Mode not found in NVS, use default
        *mode = MODE_CDC_HOST;
        ESP_LOGI(TAG, "Mode key '%s' not found in NVS namespace '%s', using default: CDC_HOST", 
                NVS_KEY_MODE, NVS_NAMESPACE);
        nvs_close(nvs_handle);
        return ESP_ERR_NVS_NOT_FOUND;  // Return error so caller knows it's a default
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading mode from NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Validate mode value
    if (mode_value > MODE_MSC_DEVICE) {
        ESP_LOGW(TAG, "Invalid mode value in NVS: %d, using default", mode_value);
        *mode = MODE_CDC_HOST;
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    } else {
        *mode = (device_mode_t)mode_value;
        ESP_LOGI(TAG, "Mode successfully read from NVS: %s (value: %d)", 
                (*mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE", mode_value);
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * @brief USB Host library handling task
 *
 * @param arg Unused
 */
static void usb_lib_task(void *arg)
{
    while (1) {
        // Start handling system events
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB: All devices freed");
            // Continue handling USB events to allow device reconnection
        }
    }
}

/**
 * @brief Initialize and run CDC Host mode
 */
static void cdc_host_init(void)
{
    device_disconnected_sem = xSemaphoreCreateBinary();
    assert(device_disconnected_sem);

    // Create queue for received data processing
    rx_data_queue = xQueueCreate(RX_DATA_QUEUE_SIZE, sizeof(rx_data_item_t));
    if (rx_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create RX data queue");
        return;
    }
    
    // Create task to process received data (lower priority than main task)
    BaseType_t rx_task_created = xTaskCreate(
        rx_processing_task,
        "rx_processor",
        4096,  // Stack size
        NULL,
        5,     // Priority (lower than main task priority of 1)
        &rx_processing_task_handle
    );
    if (rx_task_created != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create RX processing task");
        vQueueDelete(rx_data_queue);
        rx_data_queue = NULL;
        return;
    }

    // Initialize RGB LED
    ESP_LOGI(TAG, "Initializing RGB LED...");
    esp_err_t ret = rgb_led_init();
    if (ret == ESP_OK) {
        // Give RMT time to fully initialize before using it
        // Note: LED state will be set when device connects
        ESP_LOGI(TAG, "RGB LED initialized, will set state when device connects");
    } else {
        ESP_LOGW(TAG, "RGB LED initialization failed, continuing without LED");
    }

    // Initialize FatFS on internal flash
    ESP_LOGI(TAG, "Initializing FatFS...");
    ret = fatfs_init();
    if (ret == ESP_OK) {
        // Copy embedded README.md as readme.txt to storage if it doesn't exist
        if (!fatfs_file_exists("readme.txt")) {
            extern const char README_md_start[] asm("_binary_README_md_start");
            extern const char README_md_end[] asm("_binary_README_md_end");
            const size_t readme_file_size = README_md_end - README_md_start;
            
            ESP_LOGI(TAG, "Copying README.md as readme.txt to storage (%d bytes)...", readme_file_size);
            ret = fatfs_write_binary_file("readme.txt", (const uint8_t*)README_md_start, readme_file_size);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Successfully copied readme.txt to storage");
            } else {
                ESP_LOGW(TAG, "Failed to copy readme.txt to storage");
            }
        } else {
            ESP_LOGI(TAG, "readme.txt already exists in storage, skipping copy");
        }
        
        // List files in storage
        fatfs_list_files();
    } else {
        ESP_LOGW(TAG, "FatFS initialization failed, continuing without file system");
    }

    // Install USB Host driver. Should only be called once in entire application
    ESP_LOGI(TAG, "Installing USB Host");
    usb_host_config_t host_config = {};
    host_config.skip_phy_setup = false;
    host_config.intr_flags = ESP_INTR_FLAG_LOWMED;
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    // Create a task that will handle USB library events
    BaseType_t task_created = xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, NULL);
    assert(task_created == pdTRUE);

    ESP_LOGI(TAG, "Installing CDC-ACM driver");
    ESP_ERROR_CHECK(cdc_acm_host_install(NULL));

    // Register VCP drivers to VCP service
    VCP::register_driver<FT23x>();
    VCP::register_driver<CP210x>();
    VCP::register_driver<CH34x>();

    // Do everything else in a loop, so we can demonstrate USB device reconnections
    while (true) {
        const cdc_acm_host_device_config_t dev_config = {
            .connection_timeout_ms = 5000, // 5 seconds, enough time to plug the device in or experiment with timeout
            .out_buffer_size = 512,
            .in_buffer_size = 512,
            .event_cb = handle_event,
            .data_cb = handle_rx,
            .user_arg = NULL,
        };

        // You don't need to know the device's VID and PID. Just plug in any device and the VCP service will load correct (already registered) driver for the device
        device_disconnected = false;  // Reset disconnection flag for new device
        rgb_led_set_state(LED_STATE_CONNECTING);  // Set LED to MAGENTA before attempting to open
        vTaskDelay(pdMS_TO_TICKS(50));  // Give LED time to update before attempting connection
        ESP_LOGI(TAG, "Opening any VCP device...");
        auto vcp = std::unique_ptr<CdcAcmDevice>(VCP::open(&dev_config));

        if (vcp == nullptr) {
            ESP_LOGI(TAG, "Failed to open VCP device");
            // Keep LED as CONNECTING since we'll immediately try again
            // Don't set ERROR here as we're actively trying to connect
            vTaskDelay(pdMS_TO_TICKS(100));  // Small delay before retry
            continue;
        }
        vTaskDelay(10);

        ESP_LOGI(TAG, "Setting up line coding");
        cdc_acm_line_coding_t line_coding = {
            .dwDTERate = LPC21ISP_BAUDRATE,
            .bCharFormat = LPC21ISP_STOP_BITS,
            .bParityType = LPC21ISP_PARITY,
            .bDataBits = LPC21ISP_DATA_BITS,
        };
        ESP_ERROR_CHECK(vcp->line_coding_set(&line_coding));

        /*
        Now the USB-to-UART converter is configured and receiving data.
        You can use standard CDC-ACM API to interact with it. E.g.

        ESP_ERROR_CHECK(vcp->set_control_line_state(false, true));
        ESP_ERROR_CHECK(vcp->tx_blocking((uint8_t *)"Test string", 12));
        */

        // Initialize lpc21isp for bootloader sync
        ESP_LOGI(TAG, "Initializing lpc21isp");
        char baud_str[16];
        snprintf(baud_str, sizeof(baud_str), "%d", LPC21ISP_BAUDRATE);
        
        // Get the CDC handle from the device (we need to access protected member)
        // Since cdc_hdl is protected, we'll need to use a different approach
        // For now, we can use the device's tx_blocking method to verify it works
        // and pass the device pointer itself - but we need the handle
        // Let's create a workaround: store the handle when opening
        cdc_acm_dev_hdl_t cdc_handle = nullptr;
        // We need to access the protected member - create a helper class or use friend
        // For now, let's use a simple cast approach (not ideal but works)
        struct CdcAcmDeviceAccessor : public CdcAcmDevice {
            static cdc_acm_dev_hdl_t get_handle(CdcAcmDevice* dev) {
                return static_cast<CdcAcmDeviceAccessor*>(dev)->cdc_hdl;
            }
        };
        cdc_handle = CdcAcmDeviceAccessor::get_handle(vcp.get());
        
        ISP_ENVIRONMENT *isp_env = lpc21isp_esp32_init(cdc_handle, baud_str);
        
        if (isp_env != nullptr) {
            // Reset sync flag at the start of each programming attempt
            // This ensures we always attempt sync, even if a previous attempt failed
            lpc21isp_esp32_reset_sync_flag();
            
            // Set RTS high FIRST - this must be done before reset
            ESP_LOGI(TAG, "Setting RTS high - preparing for bootloader entry");
            lpc21isp_esp32_reset_to_bootloader(isp_env);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Prompt user to manually reset the target device
            ESP_LOGI(TAG, "==========================================");
            ESP_LOGI(TAG, "RTS is now HIGH");
            ESP_LOGI(TAG, "Please manually press RESET button on target device");
            ESP_LOGI(TAG, "Waiting up to 3 minutes for synchronization...");
            ESP_LOGI(TAG, "==========================================");
            rgb_led_set_state(LED_STATE_SYNCHRONIZING);
            
            // Clear any garbage data from the buffer before starting
            if (isp_env != nullptr) {
                ClearSerialPortBuffers(isp_env);
            }
            
            // Wait for the device to enter bootloader mode after reset
            // Then start sending sync characters until synchronized
            // 1800 iterations * 100ms = 180 seconds = 3 minutes
            for (int i = 0; i < 1800; i++) {  // 1800 iterations * 100ms = 3 minutes
                // Check if device was disconnected
                if (device_disconnected) {
                    ESP_LOGW(TAG, "Device disconnected during synchronization - aborting");
                    break;  // Exit loop if device disconnected
                }
                
                // Check if we're already synchronized (set by callback)
                if (lpc21isp_esp32_is_synchronized()) {
                    ESP_LOGI(TAG, "Bootloader synchronized - stopping sync character transmission");
                    break;  // Exit loop early if synchronized
                }
                
                // Verify RTS is still high (in case something overrode it)
                // Only if device is still connected
                if (!device_disconnected) {
                    cdc_handle = CdcAcmDeviceAccessor::get_handle(vcp.get());
                    if (cdc_handle) {
                        esp_err_t rts_err = cdc_acm_host_set_control_line_state(cdc_handle, false, true);  // DTR=unused, RTS=high
                        if (rts_err != ESP_OK) {
                            ESP_LOGW(TAG, "Failed to set RTS (device may be disconnected): %s", esp_err_to_name(rts_err));
                            device_disconnected = true;  // Mark as disconnected on error
                            break;
                        }
                    }
                }
                
                // Wait 100ms (1 iteration) before starting to send sync characters
                // This gives the device time to enter bootloader mode after reset
                // Then send '?' sync character every 500ms (every 5 iterations) to help bootloader sync
                // Only if device is still connected
                if (!device_disconnected && i >= 1 && (i - 1) % 5 == 0 && isp_env != nullptr) {
                    // Clear buffer periodically to avoid accumulating garbage
                    ClearSerialPortBuffers(isp_env);
                    // SendComPort may fail if device is disconnected, but we'll check flag on next iteration
                    SendComPort(isp_env, "?");
                }
                
                vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
            }
            
            // Check if we exited due to disconnection
            if (device_disconnected) {
                ESP_LOGW(TAG, "Synchronization aborted due to device disconnection");
                // Cleanup and wait for next device
                if (isp_env != nullptr) {
                    lpc21isp_esp32_cleanup(isp_env);
                }
                rgb_led_set_state(LED_STATE_IDLE);
                // Wait for device disconnection semaphore (already given, but take it to reset)
                xSemaphoreTake(device_disconnected_sem, 0);  // Non-blocking take to reset semaphore
                continue;  // Go back to waiting for next device
            }
            
            // Final buffer clear before internal sync starts
            if (isp_env != nullptr) {
                ClearSerialPortBuffers(isp_env);
            }
            
            // Check if device disconnected during sync wait
            if (device_disconnected) {
                ESP_LOGW(TAG, "Device disconnected - skipping programming");
                if (isp_env != nullptr) {
                    lpc21isp_esp32_cleanup(isp_env);
                }
                rgb_led_set_state(LED_STATE_IDLE);
                xSemaphoreTake(device_disconnected_sem, 0);  // Non-blocking take to reset semaphore
                continue;  // Go back to waiting for next device
            }
            
            // Check sync status - AppSyncing will skip if already synchronized
            // (Sync flag was reset at start, so if it's set now, it was set during the wait loop)
            if (lpc21isp_esp32_is_synchronized()) {
                ESP_LOGI(TAG, "Already synchronized - internal sync (AppSyncing) will be skipped");
            } else {
                ESP_LOGI(TAG, "Not synchronized yet - internal sync (AppSyncing) will attempt sync");
            }
            
            // Program device from hex file in storage
            // Suspend the RX processing task to prevent any logging during programming
            // This is necessary because NxpDownload corrupts memory including heap and logging system
            if (rx_processing_task_handle != NULL) {
                vTaskSuspend(rx_processing_task_handle);
                // Give a small delay to ensure task is fully suspended
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            __sync_synchronize();  // Memory barrier
            programming_in_progress = true;  // Also set flag for safety
            __sync_synchronize();  // Memory barrier to ensure write is visible
            ESP_LOGI(TAG, "Starting firmware programming from hex file...");
            rgb_led_set_state(LED_STATE_PROGRAMMING);  // Set LED to blue during programming
            int program_result = lpc21isp_esp32_program_from_hex_file(isp_env, "firmware.hex");
            __sync_synchronize();  // Memory barrier
            programming_in_progress = false;  // Clear flag
            __sync_synchronize();  // Memory barrier to ensure write is visible
            // Resume the RX processing task after programming
            if (rx_processing_task_handle != NULL) {
                vTaskResume(rx_processing_task_handle);
            }
            // Log result after a small delay to let any memory corruption settle
            vTaskDelay(pdMS_TO_TICKS(100));
            if (program_result == 0) {
                ESP_LOGI(TAG, "Firmware programming completed successfully!");
                rgb_led_set_state(LED_STATE_SYNCHRONIZED);
            } else {
                ESP_LOGE(TAG, "Firmware programming failed with error: %d", program_result);
                rgb_led_set_state(LED_STATE_ERROR);
                // Reset sync flag on failure - next attempt will need to sync again
                // The device may have exited bootloader mode after the failure
                lpc21isp_esp32_reset_sync_flag();
                ESP_LOGI(TAG, "Sync flag reset - next attempt will re-sync");
            }
            
            // Cleanup lpc21isp
            lpc21isp_esp32_cleanup(isp_env);
        } else {
            ESP_LOGE(TAG, "Failed to initialize lpc21isp");
        }

        // Send some test data
        ESP_LOGI(TAG, "Sending data through CdcAcmDevice");
        const char *test_str = "Hello from ESP32!\n";
        ESP_ERROR_CHECK(vcp->tx_blocking((uint8_t *)test_str, strlen(test_str)));

        // We are done. Wait for device disconnection and start over
        ESP_LOGI(TAG, "Done. You can reconnect the VCP device to run again.");
        xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
    }
}

/**
 * @brief Main application
 *
 * This function shows how you can use Virtual COM Port drivers
 */
extern "C" void app_main(void)
{
    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_LOGW(TAG, "NVS partition needs to be erased and reinitialized");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Log available memory at startup
    size_t free_heap = esp_get_free_heap_size();
    size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Memory Status at Startup:");
    ESP_LOGI(TAG, "  Total free heap: %zu bytes (%.2f KB)", free_heap, free_heap / 1024.0f);
    ESP_LOGI(TAG, "  Largest free block: %zu bytes (%.2f KB)", largest_free_block, largest_free_block / 1024.0f);
    ESP_LOGI(TAG, "  Minimum free heap: %zu bytes (%.2f KB)", min_free_heap, min_free_heap / 1024.0f);
    
    // Check PSRAM availability
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    if (psram_free > 0) {
        ESP_LOGI(TAG, "  PSRAM free: %zu bytes (%.2f KB / %.2f MB)", 
                 psram_free, psram_free / 1024.0f, psram_free / (1024.0f * 1024.0f));
        ESP_LOGI(TAG, "  PSRAM largest block: %zu bytes (%.2f KB)", 
                 psram_largest, psram_largest / 1024.0f);
    } else {
        ESP_LOGW(TAG, "  PSRAM: Not available");
    }
    
    // Check internal RAM
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "  Internal RAM free: %zu bytes (%.2f KB)", 
             internal_free, internal_free / 1024.0f);
    ESP_LOGI(TAG, "  Internal RAM largest block: %zu bytes (%.2f KB)", 
             internal_largest, internal_largest / 1024.0f);
    ESP_LOGI(TAG, "==========================================");
    
    // Debug: Try to open the namespace and check if it exists
    nvs_handle_t debug_handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &debug_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS namespace '%s' exists", NVS_NAMESPACE);
        // Try to read the mode key to see what's there
        uint8_t debug_mode;
        ret = nvs_get_u8(debug_handle, NVS_KEY_MODE, &debug_mode);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found mode key '%s' in NVS with value: %d (%s)", 
                    NVS_KEY_MODE, debug_mode, 
                    (debug_mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE");
        } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "Mode key '%s' does not exist in NVS namespace '%s'", 
                    NVS_KEY_MODE, NVS_NAMESPACE);
        } else {
            ESP_LOGW(TAG, "Error reading mode key: %s", esp_err_to_name(ret));
        }
        nvs_close(debug_handle);
    } else {
        ESP_LOGI(TAG, "NVS namespace '%s' does not exist yet (error: %s)", 
                NVS_NAMESPACE, esp_err_to_name(ret));
    }

    // Read mode from NVRAM
    esp_err_t mode_read_result = nvram_read_mode(&current_mode);
    if (mode_read_result == ESP_OK) {
        // Mode was successfully read from NVRAM
        ESP_LOGI(TAG, "Mode successfully loaded from NVRAM");
    } else if (mode_read_result == ESP_ERR_NVS_NOT_FOUND) {
        // Mode key doesn't exist in NVS - initialize it with default for this project
        // Since this is the CDC_HOST project, default to CDC_HOST
        current_mode = MODE_CDC_HOST;
        ESP_LOGI(TAG, "Mode key not found in NVS (first run), initializing with default: CDC_HOST");
        
        // Write the default mode to NVS so it's available for both projects
        nvs_handle_t nvs_handle;
        ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (ret == ESP_OK) {
            ret = nvs_set_u8(nvs_handle, NVS_KEY_MODE, (uint8_t)current_mode);
            if (ret == ESP_OK) {
                ret = nvs_commit(nvs_handle);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "Default mode (CDC_HOST) saved to NVRAM");
                } else {
                    ESP_LOGW(TAG, "Failed to commit default mode to NVRAM: %s", esp_err_to_name(ret));
                }
            } else {
                ESP_LOGW(TAG, "Failed to write default mode to NVRAM: %s", esp_err_to_name(ret));
            }
            nvs_close(nvs_handle);
        } else {
            ESP_LOGW(TAG, "Failed to open NVS for writing default mode: %s", esp_err_to_name(ret));
        }
    } else {
        // Other error reading from NVS
        current_mode = MODE_CDC_HOST;
        ESP_LOGE(TAG, "Failed to read mode from NVRAM (error: %s), using default: CDC_HOST", 
                esp_err_to_name(mode_read_result));
    }
    
    // Display current mode prominently
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Current Device Mode: %s", 
            (current_mode == MODE_CDC_HOST) ? "CDC_HOST" : "MSC_DEVICE");
    ESP_LOGI(TAG, "==========================================");
    
    // Initialize GPIO0 (BOOT0 button) as input with pull-up
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BOOT0_BUTTON_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // Enable pull-up (button is active low)
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "BOOT0 button (GPIO0) initialized");
    
    // Create task to monitor BOOT0 button (increased stack size to prevent overflow)
    xTaskCreate(boot0_button_task, "boot0_button", 4096, NULL, 5, NULL);
    
    // Dispatch to appropriate mode based on NVRAM setting
    if (current_mode == MODE_CDC_HOST) {
        ESP_LOGI(TAG, "Starting CDC_HOST mode...");
        cdc_host_init();
    } else {
        ESP_LOGI(TAG, "Starting MSC_DEVICE mode...");
        msc_device_init();
    }
} // app_main
} // namespace
