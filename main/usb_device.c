/**
 * @file usb_device.c
 * @brief USB Device Mode Implementation (MSC - Mass Storage Class)
 *
 * Implements USB Device MSC functionality using TinyUSB and wear leveling.
 * The ESP32-S3 appears as a USB flash drive when connected to a PC/phone.
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 2.0.0
 */

#include "usb_device.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_msc_storage.h"
#include "esp_partition.h"
#include "wear_levelling.h"
#include "tusb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "usb_device";

static bool device_initialized = false;
static wl_handle_t wl_handle = WL_INVALID_HANDLE;

/**
 * @brief Initialize USB Device Mode (MSC)
 */
esp_err_t usb_device_init(void)
{
    ESP_LOGI(TAG, "Initializing USB Device Mode (MSC)");

    if (device_initialized) {
        ESP_LOGW(TAG, "USB Device already initialized");
        return ESP_OK;
    }

    /* Step 1: Find storage partition */
    ESP_LOGI(TAG, "Step 1: Finding 'storage' partition...");
    const esp_partition_t *data_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_FAT,
        "storage"  // Look for partition named "storage"
    );

    if (data_partition == NULL) {
        ESP_LOGE(TAG, "Failed to find 'storage' partition");
        ESP_LOGE(TAG, "Ensure partitions.csv has: storage, data, fat, , 256K,");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "✓ Found partition 'storage' at offset 0x%lx, size=%lu bytes",
             data_partition->address, data_partition->size);

    /* Step 2: Erase partition to remove SPIFFS data */
    ESP_LOGI(TAG, "Step 2: Erasing partition (removing SPIFFS data)...");
    esp_err_t ret = esp_partition_erase_range(data_partition, 0, data_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase partition: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "✓ Partition erased successfully");

    /* Step 3: Mount wear leveling */
    ESP_LOGI(TAG, "Step 3: Mounting wear leveling...");
    ret = wl_mount(data_partition, &wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount wear leveling: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "✓ Wear leveling mounted (handle: %lu)", wl_handle);

    /* Step 4: Configure TinyUSB MSC storage */
    ESP_LOGI(TAG, "Step 4: Configuring TinyUSB MSC storage...");
    const tinyusb_msc_spiflash_config_t config = {
        .wl_handle = wl_handle,
        .callback_mount_changed = NULL,
        .callback_premount_changed = NULL,
        .mount_config = {
            .format_if_mount_failed = true,  // Auto-format as FAT if not already formatted
            .max_files = 5,
            .allocation_unit_size = 512,     // Match wear leveling sector size
        },
    };

    ret = tinyusb_msc_storage_init_spiflash(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MSC storage: %s", esp_err_to_name(ret));
        wl_unmount(wl_handle);
        wl_handle = WL_INVALID_HANDLE;
        return ret;
    }
    ESP_LOGI(TAG, "✓ TinyUSB MSC storage configured");

    /* Step 5: Install TinyUSB driver */
    ESP_LOGI(TAG, "Step 5: Installing TinyUSB driver...");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,  // Use default descriptor
        .string_descriptor = NULL,  // Use default strings
        .external_phy = false,
        .configuration_descriptor = NULL,
    };

    ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        wl_unmount(wl_handle);
        wl_handle = WL_INVALID_HANDLE;
        return ret;
    }
    ESP_LOGI(TAG, "✓ TinyUSB driver installed");

    device_initialized = true;
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ USB Device Mode initialized successfully");
    ESP_LOGI(TAG, "ESP32-S3 is now a USB Mass Storage Device");
    ESP_LOGI(TAG, "Connect to PC/Android to access storage partition");
    ESP_LOGI(TAG, "=================================================");

    return ESP_OK;
}

/**
 * @brief Deinitialize USB Device Mode
 */
esp_err_t usb_device_deinit(void)
{
    if (!device_initialized) {
        ESP_LOGW(TAG, "USB Device not initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing USB Device Mode...");

    /* Step 1: Uninstall TinyUSB driver */
    ESP_LOGI(TAG, "Uninstalling TinyUSB driver...");
    esp_err_t ret = tinyusb_driver_uninstall();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "TinyUSB uninstall warning: %s", esp_err_to_name(ret));
        // Continue with cleanup even if uninstall fails
    } else {
        ESP_LOGI(TAG, "✓ TinyUSB driver uninstalled");
    }

    /* Step 2: Unmount wear leveling */
    if (wl_handle != WL_INVALID_HANDLE) {
        ESP_LOGI(TAG, "Unmounting wear leveling...");
        ret = wl_unmount(wl_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to unmount wear leveling: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "✓ Wear leveling unmounted");
        }
        wl_handle = WL_INVALID_HANDLE;
    }

    device_initialized = false;
    
    /* Step 3: Critical delay for USB PHY reset */
    // Give USB PHY time to completely deinitialize before mode switch
    // This is ESSENTIAL for successful Device → Host transitions
    ESP_LOGI(TAG, "Waiting for USB PHY reset...");
    vTaskDelay(pdMS_TO_TICKS(200));
    
    ESP_LOGI(TAG, "✓ USB Device Mode deinitialized successfully");

    return ESP_OK;
}

/**
 * @brief Check if USB Device is connected to a host
 */
bool usb_device_is_connected(void)
{
    if (!device_initialized) {
        return false;
    }

    /* Check if USB is mounted (connected to host) */
    return tud_mounted();
}
