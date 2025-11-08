/**
 * @file usb_host.c
 * @brief USB Host Mode Implementation using ESP-IDF USB Host Library
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 5.0.0
 */

#include "usb_host.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "led_control.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

static const char *TAG = "usb_host";

#define USB_MOUNT_POINT "/usb"

/* USB Host state */
static bool usb_host_initialized = false;
static bool usb_device_connected = false;
static bool msc_initialized = false;
static bool safe_eject_requested = false;
static usb_host_client_handle_t client_hdl = NULL;
static usb_device_handle_t dev_hdl = NULL;
static msc_host_device_handle_t msc_device = NULL;
static msc_host_vfs_handle_t vfs_handle = NULL;
static uint8_t dev_addr = 0;
static SemaphoreHandle_t eject_mutex = NULL;

/* Forward declarations */
static void usb_host_lib_task(void *arg);
static void usb_host_client_task(void *arg);
static void usb_host_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg);
static void msc_event_cb(const msc_host_event_t *event, void *arg);

/**
 * @brief USB Host library task - handles USB host library events
 */
static void usb_host_lib_task(void *arg)
{
    ESP_LOGI(TAG, "USB Host library task started");

    while (1) {
        /* Handle USB host library events */
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

        /* Log event flags for debugging */
        if (event_flags) {
            ESP_LOGI(TAG, "Library event flags: 0x%lx", event_flags);
        }

        /* Check for new device connection */
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_LOGW(TAG, "No clients registered");
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "All devices freed");
        }
    }
}

/**
 * @brief USB Host client task - handles client events
 */
static void usb_host_client_task(void *arg)
{
    ESP_LOGI(TAG, "USB Host client task started");

    while (1) {
        /* Handle client events */
        usb_host_client_handle_events(client_hdl, portMAX_DELAY);
    }
}

/**
 * @brief MSC event callback
 */
static void msc_event_cb(const msc_host_event_t *event, void *arg)
{
    if (event->event == MSC_DEVICE_CONNECTED) {
        ESP_LOGI(TAG, "MSC device connected");
    } else if (event->event == MSC_DEVICE_DISCONNECTED) {
        ESP_LOGI(TAG, "MSC device disconnected");
    }
}

/**
 * @brief USB Host client event callback
 */
static void usb_host_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    switch (event_msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            ESP_LOGI(TAG, "New USB device connected (address: %d)", event_msg->new_dev.address);
            dev_addr = event_msg->new_dev.address;

            /* Open the device to receive DEV_GONE events */
            esp_err_t ret = usb_host_device_open(client_hdl, dev_addr, &dev_hdl);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Device opened successfully (handle: %p)", dev_hdl);
                usb_device_connected = true;
                led_control_set_state(LED_STATE_PREPARE);  /* LED to CYAN */

                /* Try to install MSC device */
                if (msc_initialized) {
                    ret = msc_host_install_device(dev_addr, &msc_device);
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "MSC device installed successfully");

                        /* Mount to VFS */
                        const esp_vfs_fat_mount_config_t mount_config = {
                            .format_if_mount_failed = false,
                            .max_files = 10,  /* Increased from 3 to allow more concurrent file operations */
                            .allocation_unit_size = 8192,
                        };

                        ESP_LOGI(TAG, "Mounting VFS with max_files=%d", mount_config.max_files);
                        ret = msc_host_vfs_register(msc_device, USB_MOUNT_POINT, &mount_config, &vfs_handle);
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "USB drive mounted at %s", USB_MOUNT_POINT);
                        } else {
                            ESP_LOGE(TAG, "Failed to mount USB drive: %s", esp_err_to_name(ret));
                            msc_host_uninstall_device(msc_device);
                            msc_device = NULL;
                        }
                    } else {
                        ESP_LOGW(TAG, "Not a MSC device or failed to install: %s", esp_err_to_name(ret));
                    }
                }
            } else {
                ESP_LOGE(TAG, "Failed to open device: %s", esp_err_to_name(ret));
            }
            break;

        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            ESP_LOGI(TAG, "USB device disconnected (handle: %p)", event_msg->dev_gone.dev_hdl);

            /* Unmount and uninstall MSC device */
            if (vfs_handle != NULL) {
                ESP_LOGI(TAG, "Unmounting USB drive...");
                msc_host_vfs_unregister(vfs_handle);
                vfs_handle = NULL;
            }

            if (msc_device != NULL) {
                msc_host_uninstall_device(msc_device);
                msc_device = NULL;
            }

            /* Close the device */
            if (dev_hdl != NULL) {
                usb_host_device_close(client_hdl, dev_hdl);
                dev_hdl = NULL;
            }

            usb_device_connected = false;
            dev_addr = 0;
            led_control_set_state(LED_STATE_IDLE);  /* LED back to IDLE */
            break;

        default:
            ESP_LOGW(TAG, "Unknown client event: %d", event_msg->event);
            break;
    }
}

/**
 * @brief Initialize USB Host Mode
 */
esp_err_t usb_host_init(void)
{
    ESP_LOGI(TAG, "Initializing USB Host Mode...");

    if (usb_host_initialized) {
        ESP_LOGW(TAG, "USB Host already initialized");
        return ESP_OK;
    }

    /* Create eject mutex */
    eject_mutex = xSemaphoreCreateMutex();
    if (eject_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create eject mutex");
        return ESP_ERR_NO_MEM;
    }

    /* Install USB Host library */
    ESP_LOGI(TAG, "Installing USB Host library...");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Host library: %s", esp_err_to_name(ret));
        vSemaphoreDelete(eject_mutex);
        eject_mutex = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "USB Host library installed successfully");

    /* Create USB Host library task */
    BaseType_t task_ret = xTaskCreate(
        usb_host_lib_task,
        "usb_host_lib",
        4096,
        NULL,
        configMAX_PRIORITIES - 2,  /* High priority for USB library events */
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB Host library task");
        usb_host_uninstall();
        return ESP_FAIL;
    }

    /* Register USB Host client */
    ESP_LOGI(TAG, "Registering USB Host client...");
    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = usb_host_client_event_cb,
            .callback_arg = NULL,
        },
    };

    ret = usb_host_client_register(&client_config, &client_hdl);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register USB Host client: %s", esp_err_to_name(ret));
        usb_host_uninstall();
        return ret;
    }

    ESP_LOGI(TAG, "USB Host client registered successfully");

    /* Create USB Host client task */
    task_ret = xTaskCreate(
        usb_host_client_task,
        "usb_host_client",
        4096,
        NULL,
        configMAX_PRIORITIES - 3,  /* Slightly lower priority than library task */
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB Host client task");
        usb_host_client_deregister(client_hdl);
        usb_host_uninstall();
        return ESP_FAIL;
    }

    /* Install MSC driver */
    ESP_LOGI(TAG, "Installing MSC driver...");
    const msc_host_driver_config_t msc_config = {
        .create_backround_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .callback = msc_event_cb,
        .callback_arg = NULL,
    };

    ret = msc_host_install(&msc_config);
    if (ret == ESP_OK) {
        msc_initialized = true;
        ESP_LOGI(TAG, "✓ MSC driver installed successfully");
    } else {
        ESP_LOGW(TAG, "Failed to install MSC driver: %s (continuing without MSC support)", esp_err_to_name(ret));
    }

    usb_host_initialized = true;
    ESP_LOGI(TAG, "✓ USB Host initialized successfully");

    return ESP_OK;
}

/**
 * @brief Deinitialize USB Host Mode
 */
esp_err_t usb_host_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing USB Host Mode...");

    if (!usb_host_initialized) {
        ESP_LOGW(TAG, "USB Host not initialized");
        return ESP_OK;
    }

    /* Uninstall MSC driver */
    if (msc_initialized) {
        ESP_LOGI(TAG, "Uninstalling MSC driver...");
        esp_err_t ret = msc_host_uninstall();
        if (ret == ESP_OK) {
            msc_initialized = false;
            ESP_LOGI(TAG, "✓ MSC driver uninstalled");
        } else {
            ESP_LOGW(TAG, "Failed to uninstall MSC driver: %s", esp_err_to_name(ret));
        }
    }

    /* Uninstall USB Host library */
    esp_err_t ret = usb_host_uninstall();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall USB Host library: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Delete eject mutex */
    if (eject_mutex != NULL) {
        vSemaphoreDelete(eject_mutex);
        eject_mutex = NULL;
    }

    usb_host_initialized = false;
    usb_device_connected = false;

    ESP_LOGI(TAG, "✓ USB Host deinitialized successfully");

    return ESP_OK;
}

/**
 * @brief Check if USB device is connected
 */
bool usb_host_is_device_connected(void)
{
    return usb_device_connected;
}

/**
 * @brief Check if USB Host is initialized
 */
bool usb_host_is_initialized(void)
{
    return usb_host_initialized;
}

/**
 * @brief Get USB mount point
 */
const char* usb_host_get_mount_point(void)
{
    return (vfs_handle != NULL) ? USB_MOUNT_POINT : NULL;
}

/**
 * @brief Sync filesystem to ensure all data is written to USB drive
 */
esp_err_t usb_host_sync_filesystem(void)
{
    if (vfs_handle == NULL) {
        ESP_LOGW(TAG, "Cannot sync: USB drive not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Syncing filesystem...");

    /* VFS unmount will automatically sync, but we add a delay to ensure
     * any pending writes are completed */
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "✓ Filesystem sync delay completed");
    return ESP_OK;
}

/**
 * @brief Safely eject USB drive (sync + unmount)
 */
esp_err_t usb_host_safe_eject(void)
{
    if (eject_mutex == NULL) {
        ESP_LOGE(TAG, "Eject mutex not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Take mutex to prevent concurrent eject operations */
    if (xSemaphoreTake(eject_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire eject mutex");
        return ESP_ERR_TIMEOUT;
    }

    if (vfs_handle == NULL) {
        ESP_LOGW(TAG, "Cannot eject: USB drive not mounted");
        xSemaphoreGive(eject_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Safe Eject: Starting...");
    ESP_LOGI(TAG, "=================================================");

    /* Step 1: Sync filesystem */
    ESP_LOGI(TAG, "Step 1: Syncing filesystem...");
    led_control_set_state(LED_STATE_SYNC);  /* LED to MAGENTA */

    esp_err_t ret = usb_host_sync_filesystem();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to sync filesystem: %s", esp_err_to_name(ret));
        xSemaphoreGive(eject_mutex);
        return ret;
    }

    /* Step 2: Unmount VFS */
    ESP_LOGI(TAG, "Step 2: Unmounting VFS...");
    ret = msc_host_vfs_unregister(vfs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount VFS: %s", esp_err_to_name(ret));
        led_control_set_state(LED_STATE_ERROR);  /* LED to RED */
        xSemaphoreGive(eject_mutex);
        return ret;
    }
    vfs_handle = NULL;
    ESP_LOGI(TAG, "✓ VFS unmounted");

    /* Step 3: Uninstall MSC device */
    ESP_LOGI(TAG, "Step 3: Uninstalling MSC device...");
    if (msc_device != NULL) {
        ret = msc_host_uninstall_device(msc_device);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to uninstall MSC device: %s", esp_err_to_name(ret));
            led_control_set_state(LED_STATE_ERROR);  /* LED to RED */
            xSemaphoreGive(eject_mutex);
            return ret;
        }
        msc_device = NULL;
        ESP_LOGI(TAG, "✓ MSC device uninstalled");
    }

    /* Step 4: Close USB device */
    ESP_LOGI(TAG, "Step 4: Closing USB device...");
    if (dev_hdl != NULL) {
        ret = usb_host_device_close(client_hdl, dev_hdl);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to close USB device: %s", esp_err_to_name(ret));
            led_control_set_state(LED_STATE_ERROR);  /* LED to RED */
            xSemaphoreGive(eject_mutex);
            return ret;
        }
        dev_hdl = NULL;
        ESP_LOGI(TAG, "✓ USB device closed");
    }

    usb_device_connected = false;
    dev_addr = 0;
    safe_eject_requested = false;

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ Safe Eject: COMPLETE");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "USB drive can now be safely removed");

    led_control_set_state(LED_STATE_SUCCESS);  /* LED to GREEN SOLID */
    vTaskDelay(pdMS_TO_TICKS(2000));  /* Show success for 2 seconds */
    led_control_set_state(LED_STATE_IDLE);  /* Back to IDLE */

    xSemaphoreGive(eject_mutex);
    return ESP_OK;
}

/**
 * @brief Request safe eject (non-blocking)
 */
esp_err_t usb_host_request_safe_eject(void)
{
    if (vfs_handle == NULL) {
        ESP_LOGW(TAG, "Cannot request eject: USB drive not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    safe_eject_requested = true;
    ESP_LOGI(TAG, "Safe eject requested");
    return ESP_OK;
}

/**
 * @brief Check if safe eject is requested
 */
bool usb_host_is_eject_requested(void)
{
    return safe_eject_requested;
}

/**
 * @brief Read file from USB drive
 */
esp_err_t usb_host_read_file(const char* file_path, char* buffer, size_t buffer_size, size_t* bytes_read)
{
    if (vfs_handle == NULL) {
        ESP_LOGE(TAG, "Cannot read file: USB drive not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (file_path == NULL || buffer == NULL || bytes_read == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    /* Build full path */
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", USB_MOUNT_POINT, file_path);

    ESP_LOGI(TAG, "Reading file: %s", full_path);

    /* Open file for reading */
    FILE* f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", full_path);
        return ESP_FAIL;
    }

    /* Read file contents */
    *bytes_read = fread(buffer, 1, buffer_size - 1, f);
    buffer[*bytes_read] = '\0';  /* Null-terminate */

    fclose(f);

    ESP_LOGI(TAG, "✓ Read %d bytes from %s", *bytes_read, file_path);
    return ESP_OK;
}

/**
 * @brief Write file to USB drive
 */
esp_err_t usb_host_write_file(const char* file_path, const char* data, size_t data_size)
{
    if (vfs_handle == NULL) {
        ESP_LOGE(TAG, "Cannot write file: USB drive not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (file_path == NULL || data == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    /* Build full path */
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", USB_MOUNT_POINT, file_path);

    ESP_LOGI(TAG, "Writing file: %s (%d bytes)", full_path, data_size);

    /* Check if mount point is accessible */
    struct stat st;
    if (stat(USB_MOUNT_POINT, &st) != 0) {
        ESP_LOGE(TAG, "Mount point %s not accessible", USB_MOUNT_POINT);
        return ESP_ERR_INVALID_STATE;
    }

    /* Open file for writing - try "w+" mode for FAT compatibility */
    FILE* f = fopen(full_path, "w+");
    if (f == NULL) {
        int err = errno;
        ESP_LOGE(TAG, "Failed to open file for writing: %s", full_path);
        ESP_LOGE(TAG, "Error code: %d (%s)", err, strerror(err));
        if (err == EROFS) {
            ESP_LOGE(TAG, "File system is READ-ONLY!");
        } else if (err == EACCES) {
            ESP_LOGE(TAG, "Permission denied - file may be write-protected");
        } else if (err == ENFILE) {
            ESP_LOGE(TAG, "Too many open files - increase max_files in VFS mount config");
        } else if (err == ENOSPC) {
            ESP_LOGE(TAG, "No space left on device");
        } else if (err == EINVAL) {
            ESP_LOGE(TAG, "Invalid argument - FAT filesystem may not support this operation");
            ESP_LOGE(TAG, "This could be due to:");
            ESP_LOGE(TAG, "  1. File system mounted read-only");
            ESP_LOGE(TAG, "  2. Invalid filename for FAT32");
            ESP_LOGE(TAG, "  3. VFS configuration issue");
        }
        return ESP_FAIL;
    }

    /* Write data */
    size_t written = fwrite(data, 1, data_size, f);
    fclose(f);

    if (written != data_size) {
        ESP_LOGE(TAG, "Write failed: wrote %d bytes, expected %d", written, data_size);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "✓ Wrote %d bytes to %s", written, file_path);
    return ESP_OK;
}

/**
 * @brief Get file size
 */
esp_err_t usb_host_get_file_size(const char* file_path, size_t* file_size)
{
    if (vfs_handle == NULL) {
        ESP_LOGE(TAG, "Cannot get file size: USB drive not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (file_path == NULL || file_size == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    /* Build full path */
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", USB_MOUNT_POINT, file_path);

    /* Get file stats */
    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "Failed to get file stats: %s", full_path);
        return ESP_FAIL;
    }

    *file_size = st.st_size;
    ESP_LOGI(TAG, "File size: %s = %d bytes", file_path, *file_size);
    return ESP_OK;
}
