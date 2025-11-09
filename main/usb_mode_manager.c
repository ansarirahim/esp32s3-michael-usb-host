/**
 * @file usb_mode_manager.c
 * @brief USB Mode Manager - Runtime Mode Switching Implementation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#include "usb_mode_manager.h"
#include "usb_host.h"
#include "usb_device.h"
#include "led_control.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "mode_manager";

/* Mode manager state */
static bool manager_initialized = false;
static usb_mode_t current_mode = USB_MODE_HOST;
static uint32_t mode_switches = 0;
static uint32_t switch_failures = 0;
static uint64_t last_switch_time_ms = 0;
static uint64_t init_time_ms = 0;
static SemaphoreHandle_t mode_switch_mutex = NULL;

/* Retry configuration */
#define MODE_SWITCH_MAX_RETRIES     3
#define MODE_SWITCH_RETRY_DELAY_MS  500

/**
 * @brief Initialize USB Mode Manager
 */
esp_err_t usb_mode_manager_init(usb_mode_t initial_mode)
{
    if (manager_initialized) {
        ESP_LOGW(TAG, "Mode manager already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing USB Mode Manager (initial mode: %s)", 
             initial_mode == USB_MODE_HOST ? "HOST" : "DEVICE");

    /* Create mutex for mode switching */
    mode_switch_mutex = xSemaphoreCreateMutex();
    if (mode_switch_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mode switch mutex");
        return ESP_ERR_NO_MEM;
    }

    /* Initialize USB in the selected mode */
    esp_err_t ret = ESP_OK;
    current_mode = initial_mode;

    if (initial_mode == USB_MODE_HOST) {
        ESP_LOGI(TAG, "Initializing USB Host mode...");
        ret = usb_host_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize USB Host: %s", esp_err_to_name(ret));
            vSemaphoreDelete(mode_switch_mutex);
            mode_switch_mutex = NULL;
            return ret;
        }
        ESP_LOGI(TAG, "USB Host mode initialized successfully");
    } else {
        ESP_LOGI(TAG, "Initializing USB Device mode...");
        ret = usb_device_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize USB Device: %s", esp_err_to_name(ret));
            vSemaphoreDelete(mode_switch_mutex);
            mode_switch_mutex = NULL;
            return ret;
        }
        ESP_LOGI(TAG, "USB Device mode initialized successfully");
    }

    manager_initialized = true;
    mode_switches = 0;
    switch_failures = 0;
    init_time_ms = esp_timer_get_time() / 1000;
    last_switch_time_ms = init_time_ms;

    ESP_LOGI(TAG, "USB Mode Manager initialized successfully");
    return ESP_OK;
}

/**
 * @brief Deinitialize USB Mode Manager
 */
esp_err_t usb_mode_manager_deinit(void)
{
    if (!manager_initialized) {
        ESP_LOGW(TAG, "Mode manager not initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing USB Mode Manager...");

    /* Deinitialize current USB mode */
    if (current_mode == USB_MODE_HOST) {
        usb_host_deinit();
    } else {
        usb_device_deinit();
    }

    /* Delete mutex */
    if (mode_switch_mutex != NULL) {
        vSemaphoreDelete(mode_switch_mutex);
        mode_switch_mutex = NULL;
    }

    manager_initialized = false;
    ESP_LOGI(TAG, "USB Mode Manager deinitialized");
    return ESP_OK;
}

/**
 * @brief Switch USB mode
 */
esp_err_t usb_mode_manager_switch(usb_mode_t new_mode)
{
    if (!manager_initialized) {
        ESP_LOGE(TAG, "Mode manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (new_mode == current_mode) {
        ESP_LOGW(TAG, "Already in %s mode", new_mode == USB_MODE_HOST ? "HOST" : "DEVICE");
        return ESP_OK;
    }

    /* Take mutex */
    if (xSemaphoreTake(mode_switch_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mode switch mutex (timeout)");
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Mode switch requested: %s -> %s", 
             current_mode == USB_MODE_HOST ? "HOST" : "DEVICE",
             new_mode == USB_MODE_HOST ? "HOST" : "DEVICE");
    ESP_LOGI(TAG, "=================================================");

    /* Check if mode switching is allowed */
    if (!usb_mode_manager_can_switch()) {
        ESP_LOGW(TAG, "Mode switch denied: USB operation in progress");
        led_control_set_state(LED_STATE_ERROR);
        vTaskDelay(pdMS_TO_TICKS(2000));  /* Show error for 2 seconds */
        led_control_set_state(current_mode == USB_MODE_HOST ? LED_STATE_IDLE : LED_STATE_DEVICE_IDLE);
        xSemaphoreGive(mode_switch_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    /* Set LED to mode switching state (orange fast blink) */
    led_control_set_state(LED_STATE_MODE_SWITCHING);

    esp_err_t ret = ESP_OK;
    int retry_count = 0;

    /* Retry loop for mode switching */
    while (retry_count < MODE_SWITCH_MAX_RETRIES) {
        /* Step 1: Deinitialize current mode */
        ESP_LOGI(TAG, "Step 1: Deinitializing %s mode...", 
                 current_mode == USB_MODE_HOST ? "HOST" : "DEVICE");

        if (current_mode == USB_MODE_HOST) {
            ret = usb_host_deinit();
        } else {
            ret = usb_device_deinit();
        }

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to deinitialize %s mode (retry %d/%d): %s",
                     current_mode == USB_MODE_HOST ? "HOST" : "DEVICE",
                     retry_count + 1, MODE_SWITCH_MAX_RETRIES,
                     esp_err_to_name(ret));
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(MODE_SWITCH_RETRY_DELAY_MS));
            continue;
        }

        ESP_LOGI(TAG, "✓ %s mode deinitialized", current_mode == USB_MODE_HOST ? "HOST" : "DEVICE");

        /* Step 2: Wait for USB PHY to stabilize */
        ESP_LOGI(TAG, "Step 2: Waiting for USB PHY to stabilize...");
        vTaskDelay(pdMS_TO_TICKS(1000));  /* Increased from 500ms to 1000ms */

        /* Step 3: Initialize new mode */
        ESP_LOGI(TAG, "Step 3: Initializing %s mode...", 
                 new_mode == USB_MODE_HOST ? "HOST" : "DEVICE");

        if (new_mode == USB_MODE_HOST) {
            ret = usb_host_init();
        } else {
            ret = usb_device_init();
        }

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to initialize %s mode (retry %d/%d): %s",
                     new_mode == USB_MODE_HOST ? "HOST" : "DEVICE",
                     retry_count + 1, MODE_SWITCH_MAX_RETRIES,
                     esp_err_to_name(ret));
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(MODE_SWITCH_RETRY_DELAY_MS));
            continue;
        }

        ESP_LOGI(TAG, "✓ %s mode initialized", new_mode == USB_MODE_HOST ? "HOST" : "DEVICE");

        /* Success! */
        break;
    }

    /* Check if mode switch succeeded */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mode switch failed after %d retries, reverting to %s mode",
                 MODE_SWITCH_MAX_RETRIES,
                 current_mode == USB_MODE_HOST ? "HOST" : "DEVICE");

        /* Revert to original mode */
        if (current_mode == USB_MODE_HOST) {
            usb_host_init();
        } else {
            usb_device_init();
        }

        switch_failures++;
        led_control_set_state(LED_STATE_ERROR);
        vTaskDelay(pdMS_TO_TICKS(2000));  /* Show error for 2 seconds */
        led_control_set_state(current_mode == USB_MODE_HOST ? LED_STATE_IDLE : LED_STATE_DEVICE_IDLE);
        xSemaphoreGive(mode_switch_mutex);
        return ESP_FAIL;
    }

    /* Update state */
    current_mode = new_mode;
    mode_switches++;
    last_switch_time_ms = esp_timer_get_time() / 1000;

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ Mode switch complete: %s", new_mode == USB_MODE_HOST ? "HOST" : "DEVICE");
    ESP_LOGI(TAG, "=================================================");

    /* Set LED to new mode's idle state */
    if (new_mode == USB_MODE_HOST) {
        led_control_set_state(LED_STATE_IDLE);  /* Green slow blink */
    } else {
        led_control_set_state(LED_STATE_DEVICE_IDLE);  /* Blue slow blink */
    }

    xSemaphoreGive(mode_switch_mutex);
    return ESP_OK;
}

/**
 * @brief Toggle USB mode
 */
esp_err_t usb_mode_manager_toggle(void)
{
    usb_mode_t new_mode = (current_mode == USB_MODE_HOST) ? USB_MODE_DEVICE : USB_MODE_HOST;
    return usb_mode_manager_switch(new_mode);
}

/**
 * @brief Get current USB mode
 */
usb_mode_t usb_mode_manager_get_mode(void)
{
    return current_mode;
}

/**
 * @brief Get USB mode as string
 */
const char* usb_mode_manager_get_mode_name(usb_mode_t mode)
{
    return (mode == USB_MODE_HOST) ? "HOST" : "DEVICE";
}

/**
 * @brief Check if mode switching is allowed
 */
bool usb_mode_manager_can_switch(void)
{
    /* Check if USB Host is mounted (file operation in progress) */
    if (current_mode == USB_MODE_HOST && usb_host_is_mounted()) {
        ESP_LOGW(TAG, "Cannot switch: USB drive is mounted");
        return false;
    }

    /* Check if USB Device is connected to PC */
    if (current_mode == USB_MODE_DEVICE && usb_device_is_connected()) {
        ESP_LOGW(TAG, "Cannot switch: USB Device is connected to PC");
        return false;
    }

    return true;
}

/**
 * @brief Get mode manager statistics
 */
esp_err_t usb_mode_manager_get_stats(usb_mode_stats_t* stats)
{
    if (stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!manager_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    stats->current_mode = current_mode;
    stats->mode_switches = mode_switches;
    stats->switch_failures = switch_failures;
    stats->uptime_ms = (esp_timer_get_time() / 1000) - init_time_ms;
    stats->last_switch_time_ms = last_switch_time_ms - init_time_ms;

    return ESP_OK;
}

/**
 * @brief Check if USB Mode Manager is initialized
 */
bool usb_mode_manager_is_initialized(void)
{
    return manager_initialized;
}

