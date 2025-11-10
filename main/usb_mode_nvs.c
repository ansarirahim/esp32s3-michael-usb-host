/**
 * @file usb_mode_nvs.c
 * @brief USB Mode NVS Storage
 * 
 * Manages USB mode persistence in NVS (Non-Volatile Storage).
 * Allows mode to persist across reboots.
 * 
 * Based on Michael's reference implementation.
 */

#include "usb_mode_nvs.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "usb_mode_nvs";

/* NVS Configuration */
#define NVS_NAMESPACE "usb_config"
#define NVS_KEY_MODE "mode"

/**
 * @brief Initialize NVS
 */
esp_err_t usb_mode_nvs_init(void)
{
    ESP_LOGI(TAG, "Initializing NVS...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated and needs to be erased */
        ESP_LOGW(TAG, "NVS partition needs to be erased, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ NVS initialized successfully");
    return ESP_OK;
}

/**
 * @brief Write USB mode to NVS
 */
esp_err_t usb_mode_nvs_write(usb_mode_t mode)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Writing mode to NVS: %s", mode == USB_MODE_HOST ? "HOST" : "DEVICE");
    
    /* Open NVS */
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Write mode */
    ret = nvs_set_u8(nvs_handle, NVS_KEY_MODE, (uint8_t)mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error writing mode to NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    /* Commit */
    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "✓ Mode written to NVS successfully");
    return ESP_OK;
}

/**
 * @brief Read USB mode from NVS
 */
esp_err_t usb_mode_nvs_read(usb_mode_t *mode)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Reading mode from NVS...");
    
    /* Try to open with READWRITE first (will create namespace if it doesn't exist) */
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        /* If that fails, try READONLY */
        ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Error opening NVS handle: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    
    /* Read mode */
    uint8_t mode_value;
    ret = nvs_get_u8(nvs_handle, NVS_KEY_MODE, &mode_value);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* Mode not found in NVS, use default */
        *mode = USB_MODE_HOST;
        ESP_LOGI(TAG, "Mode key '%s' not found in NVS namespace '%s', using default: HOST", 
                NVS_KEY_MODE, NVS_NAMESPACE);
        nvs_close(nvs_handle);
        return ESP_ERR_NVS_NOT_FOUND;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error reading mode from NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    /* Validate mode value */
    if (mode_value != USB_MODE_HOST && mode_value != USB_MODE_DEVICE) {
        ESP_LOGW(TAG, "Invalid mode value in NVS: %d, using default: HOST", mode_value);
        *mode = USB_MODE_HOST;
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_ARG;
    }
    
    *mode = (usb_mode_t)mode_value;
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "✓ Mode read from NVS: %s", *mode == USB_MODE_HOST ? "HOST" : "DEVICE");
    return ESP_OK;
}

/**
 * @brief Toggle USB mode in NVS and return new mode
 */
esp_err_t usb_mode_nvs_toggle(usb_mode_t *new_mode)
{
    usb_mode_t current_mode;
    esp_err_t ret;
    
    /* Read current mode */
    ret = usb_mode_nvs_read(&current_mode);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read current mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Toggle mode */
    *new_mode = (current_mode == USB_MODE_HOST) ? USB_MODE_DEVICE : USB_MODE_HOST;
    
    ESP_LOGI(TAG, "Toggling mode: %s -> %s", 
             current_mode == USB_MODE_HOST ? "HOST" : "DEVICE",
             *new_mode == USB_MODE_HOST ? "HOST" : "DEVICE");
    
    /* Write new mode */
    ret = usb_mode_nvs_write(*new_mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write new mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

