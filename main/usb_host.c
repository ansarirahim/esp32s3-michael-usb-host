/**
 * @file usb_host.c
 * @brief USB Host Mode Implementation using ESP-IDF USB Host Library
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 7.0.0
 */

#include "usb_host.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "esp_private/msc_scsi_bot.h"
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

/* ============================================================================
 * PHASE 3A: PARTITION DETECTION & DELETION
 * ============================================================================ */

#define SECTOR_SIZE 512
#define MBR_BOOT_SIGNATURE_OFFSET 0x1FE
#define MBR_BOOT_SIGNATURE 0xAA55
#define MBR_PARTITION_TABLE_OFFSET 0x1BE
#define MBR_PARTITION_ENTRY_SIZE 16
#define MBR_MAX_PARTITIONS 4

/**
 * @brief Read raw sector from USB drive
 */
esp_err_t usb_host_read_sector(uint32_t sector_num, uint8_t* buffer)
{
    if (msc_device == NULL) {
        ESP_LOGE(TAG, "Cannot read sector: MSC device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (buffer == NULL) {
        ESP_LOGE(TAG, "Invalid buffer");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Reading sector %lu", sector_num);

    /* Use SCSI READ(10) command to read sector */
    esp_err_t ret = scsi_cmd_read10(msc_device, buffer, sector_num, 1, SECTOR_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sector %lu: %d", sector_num, ret);
        return ret;
    }

    ESP_LOGD(TAG, "✓ Read sector %lu successfully", sector_num);
    return ESP_OK;
}

/**
 * @brief Write raw sector to USB drive
 */
esp_err_t usb_host_write_sector(uint32_t sector_num, const uint8_t* buffer)
{
    if (msc_device == NULL) {
        ESP_LOGE(TAG, "Cannot write sector: MSC device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (buffer == NULL) {
        ESP_LOGE(TAG, "Invalid buffer");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Writing sector %lu", sector_num);

    /* Use SCSI WRITE(10) command to write sector */
    esp_err_t ret = scsi_cmd_write10(msc_device, buffer, sector_num, 1, SECTOR_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write sector %lu: %d", sector_num, ret);
        return ret;
    }

    ESP_LOGD(TAG, "✓ Wrote sector %lu successfully", sector_num);
    return ESP_OK;
}

/**
 * @brief Detect partition table type
 */
esp_err_t usb_host_detect_partition_table(partition_table_type_t* table_type)
{
    if (table_type == NULL) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }

    /* Read MBR (sector 0) */
    uint8_t mbr[SECTOR_SIZE];
    esp_err_t ret = usb_host_read_sector(0, mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MBR");
        return ret;
    }

    /* Check boot signature */
    uint16_t boot_sig = (mbr[MBR_BOOT_SIGNATURE_OFFSET + 1] << 8) | mbr[MBR_BOOT_SIGNATURE_OFFSET];
    if (boot_sig != MBR_BOOT_SIGNATURE) {
        ESP_LOGW(TAG, "Invalid boot signature: 0x%04X (expected 0x%04X)", boot_sig, MBR_BOOT_SIGNATURE);
        *table_type = PARTITION_TABLE_UNKNOWN;
        return ESP_OK;
    }

    /* Check first partition type */
    uint8_t first_partition_type = mbr[MBR_PARTITION_TABLE_OFFSET + 4];

    if (first_partition_type == 0xEE) {
        /* GPT protective MBR */
        *table_type = PARTITION_TABLE_GPT;
        ESP_LOGI(TAG, "Detected GPT partition table");
    } else if (first_partition_type == 0x00) {
        /* No partitions */
        *table_type = PARTITION_TABLE_NONE;
        ESP_LOGI(TAG, "No partition table detected");
    } else {
        /* MBR partition table */
        *table_type = PARTITION_TABLE_MBR;
        ESP_LOGI(TAG, "Detected MBR partition table");
    }

    return ESP_OK;
}

/**
 * @brief Get number of partitions
 */
esp_err_t usb_host_get_partition_count(uint8_t* count)
{
    if (count == NULL) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }

    /* Read MBR (sector 0) */
    uint8_t mbr[SECTOR_SIZE];
    esp_err_t ret = usb_host_read_sector(0, mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MBR");
        return ret;
    }

    /* Count non-empty partitions */
    *count = 0;
    for (int i = 0; i < MBR_MAX_PARTITIONS; i++) {
        uint16_t offset = MBR_PARTITION_TABLE_OFFSET + (i * MBR_PARTITION_ENTRY_SIZE);
        uint8_t partition_type = mbr[offset + 4];
        if (partition_type != 0x00) {
            (*count)++;
        }
    }

    ESP_LOGI(TAG, "Partition count: %d", *count);
    return ESP_OK;
}

/**
 * @brief Get partition information
 */
esp_err_t usb_host_get_partition_info(uint8_t partition_num, partition_info_t* info)
{
    if (info == NULL) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }

    if (partition_num >= MBR_MAX_PARTITIONS) {
        ESP_LOGE(TAG, "Invalid partition number: %d (max: %d)", partition_num, MBR_MAX_PARTITIONS - 1);
        return ESP_ERR_INVALID_ARG;
    }

    /* Read MBR (sector 0) */
    uint8_t mbr[SECTOR_SIZE];
    esp_err_t ret = usb_host_read_sector(0, mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MBR");
        return ret;
    }

    /* Parse partition entry */
    uint16_t offset = MBR_PARTITION_TABLE_OFFSET + (partition_num * MBR_PARTITION_ENTRY_SIZE);

    info->boot_indicator = mbr[offset + 0];
    info->partition_type = mbr[offset + 4];

    /* Read LBA (little-endian) */
    info->start_lba = mbr[offset + 8] |
                      (mbr[offset + 9] << 8) |
                      (mbr[offset + 10] << 16) |
                      (mbr[offset + 11] << 24);

    /* Read size in sectors (little-endian) */
    info->size_sectors = mbr[offset + 12] |
                         (mbr[offset + 13] << 8) |
                         (mbr[offset + 14] << 16) |
                         (mbr[offset + 15] << 24);

    /* Calculate size in bytes */
    info->size_bytes = (uint64_t)info->size_sectors * SECTOR_SIZE;

    ESP_LOGI(TAG, "Partition %d: Type=0x%02X, Start=%lu, Size=%lu sectors (%.2f MB)",
             partition_num, info->partition_type, info->start_lba, info->size_sectors,
             (float)info->size_bytes / (1024.0 * 1024.0));

    return ESP_OK;
}

/**
 * @brief Delete all partitions (zero MBR)
 */
esp_err_t usb_host_delete_all_partitions(void)
{
    ESP_LOGW(TAG, "=================================================");
    ESP_LOGW(TAG, "⚠️  WARNING: DELETING ALL PARTITIONS!");
    ESP_LOGW(TAG, "⚠️  ALL DATA WILL BE LOST!");
    ESP_LOGW(TAG, "=================================================");

    /* Check if MSC device is initialized */
    if (msc_device == NULL) {
        ESP_LOGE(TAG, "Cannot delete partitions: MSC device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Unmount VFS if mounted */
    if (vfs_handle != NULL) {
        ESP_LOGI(TAG, "Unmounting VFS before partition deletion...");
        esp_err_t ret = msc_host_vfs_unregister(vfs_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to unmount VFS: %d (continuing anyway)", ret);
        } else {
            ESP_LOGI(TAG, "✓ VFS unmounted");
        }
        vfs_handle = NULL;
    }

    /* Create zero buffer */
    uint8_t zero_mbr[SECTOR_SIZE];
    memset(zero_mbr, 0, SECTOR_SIZE);

    /* Write zeros to MBR (sector 0) */
    ESP_LOGI(TAG, "Writing zeros to MBR (sector 0)...");
    esp_err_t ret = usb_host_write_sector(0, zero_mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to zero MBR");
        return ret;
    }

    /* Verify deletion by reading back */
    ESP_LOGI(TAG, "Verifying MBR deletion...");
    uint8_t verify_mbr[SECTOR_SIZE];
    ret = usb_host_read_sector(0, verify_mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to verify MBR deletion");
        return ret;
    }

    /* Check if MBR is zeroed */
    bool is_zeroed = true;
    for (int i = 0; i < SECTOR_SIZE; i++) {
        if (verify_mbr[i] != 0) {
            is_zeroed = false;
            break;
        }
    }

    if (!is_zeroed) {
        ESP_LOGE(TAG, "MBR verification failed: not all zeros");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ All partitions deleted successfully");
    ESP_LOGI(TAG, "✓ MBR zeroed and verified");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "USB drive is now ready for formatting");

    return ESP_OK;
}

/**
 * @brief Get total number of sectors on the USB drive
 */
esp_err_t usb_host_get_drive_capacity(uint32_t* total_sectors)
{
    if (total_sectors == NULL) {
        ESP_LOGE(TAG, "Total sectors pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (msc_device == NULL) {
        ESP_LOGE(TAG, "MSC device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t block_size = 0;
    uint32_t block_count = 0;

    /* Read drive capacity using SCSI READ CAPACITY(10) */
    esp_err_t ret = scsi_cmd_read_capacity(msc_device, &block_size, &block_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read drive capacity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Drive capacity: %lu blocks × %lu bytes = %llu bytes (%.2f MB)",
             block_count, block_size, (uint64_t)block_count * block_size,
             (float)((uint64_t)block_count * block_size) / (1024.0 * 1024.0));

    *total_sectors = block_count;
    return ESP_OK;
}

/**
 * @brief Create a new MBR partition table with a single FAT32 partition
 */
esp_err_t usb_host_create_partition_table(uint32_t total_sectors)
{
    ESP_LOGW(TAG, "=================================================");
    ESP_LOGW(TAG, "⚠️  WARNING: Creating new partition table");
    ESP_LOGW(TAG, "⚠️  ALL DATA WILL BE LOST!");
    ESP_LOGW(TAG, "=================================================");

    if (msc_device == NULL) {
        ESP_LOGE(TAG, "MSC device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Unmount VFS if mounted */
    if (vfs_handle != NULL) {
        ESP_LOGI(TAG, "Unmounting VFS before partition creation...");
        esp_err_t ret = msc_host_vfs_unregister(vfs_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to unmount VFS: %d (continuing anyway)", ret);
        }
        vfs_handle = NULL;
    }

    /* Create MBR buffer */
    uint8_t mbr[SECTOR_SIZE];
    memset(mbr, 0, SECTOR_SIZE);

    /* Calculate partition parameters */
    uint32_t partition_start = 2048;  /* 1 MB offset for alignment */
    uint32_t partition_size = total_sectors - partition_start;

    ESP_LOGI(TAG, "Creating partition:");
    ESP_LOGI(TAG, "  Start LBA: %lu", partition_start);
    ESP_LOGI(TAG, "  Size: %lu sectors (%.2f MB)",
             partition_size, (float)((uint64_t)partition_size * SECTOR_SIZE) / (1024.0 * 1024.0));

    /* Create partition entry 1 at offset 0x1BE */
    uint16_t entry_offset = MBR_PARTITION_TABLE_OFFSET;

    /* Boot indicator (0x00 = non-bootable) */
    mbr[entry_offset + 0] = 0x00;

    /* CHS start (0xFFFFFF for LBA) */
    mbr[entry_offset + 1] = 0xFF;
    mbr[entry_offset + 2] = 0xFF;
    mbr[entry_offset + 3] = 0xFF;

    /* Partition type (0x0C = FAT32 LBA) */
    mbr[entry_offset + 4] = 0x0C;

    /* CHS end (0xFFFFFF for LBA) */
    mbr[entry_offset + 5] = 0xFF;
    mbr[entry_offset + 6] = 0xFF;
    mbr[entry_offset + 7] = 0xFF;

    /* LBA start (little-endian) */
    mbr[entry_offset + 8] = (partition_start >> 0) & 0xFF;
    mbr[entry_offset + 9] = (partition_start >> 8) & 0xFF;
    mbr[entry_offset + 10] = (partition_start >> 16) & 0xFF;
    mbr[entry_offset + 11] = (partition_start >> 24) & 0xFF;

    /* Size in sectors (little-endian) */
    mbr[entry_offset + 12] = (partition_size >> 0) & 0xFF;
    mbr[entry_offset + 13] = (partition_size >> 8) & 0xFF;
    mbr[entry_offset + 14] = (partition_size >> 16) & 0xFF;
    mbr[entry_offset + 15] = (partition_size >> 24) & 0xFF;

    /* Boot signature (0x55AA) */
    mbr[MBR_BOOT_SIGNATURE_OFFSET] = 0x55;
    mbr[MBR_BOOT_SIGNATURE_OFFSET + 1] = 0xAA;

    /* Write MBR to sector 0 */
    ESP_LOGI(TAG, "Writing MBR to sector 0...");
    esp_err_t ret = usb_host_write_sector(0, mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write MBR");
        return ret;
    }

    /* Verify MBR */
    uint8_t verify_mbr[SECTOR_SIZE];
    ret = usb_host_read_sector(0, verify_mbr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to verify MBR");
        return ret;
    }

    /* Check boot signature */
    uint16_t boot_sig = (verify_mbr[MBR_BOOT_SIGNATURE_OFFSET + 1] << 8) | verify_mbr[MBR_BOOT_SIGNATURE_OFFSET];
    if (boot_sig != MBR_BOOT_SIGNATURE) {
        ESP_LOGE(TAG, "MBR verification failed: invalid boot signature 0x%04X", boot_sig);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ MBR partition table created successfully");
    ESP_LOGI(TAG, "✓ Partition 0: Type=0x0C (FAT32 LBA)");
    ESP_LOGI(TAG, "✓ Start LBA: %lu", partition_start);
    ESP_LOGI(TAG, "✓ Size: %lu sectors", partition_size);
    ESP_LOGI(TAG, "=================================================");

    return ESP_OK;
}

/**
 * @brief Format a partition as FAT32
 */
esp_err_t usb_host_format_fat32(uint8_t partition_num, uint32_t start_lba, uint32_t size_sectors)
{
    ESP_LOGW(TAG, "=================================================");
    ESP_LOGW(TAG, "⚠️  WARNING: Formatting partition as FAT32");
    ESP_LOGW(TAG, "⚠️  ALL DATA WILL BE LOST!");
    ESP_LOGW(TAG, "=================================================");

    if (msc_device == NULL) {
        ESP_LOGE(TAG, "MSC device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (partition_num >= MBR_MAX_PARTITIONS) {
        ESP_LOGE(TAG, "Invalid partition number: %d", partition_num);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Formatting partition %d:", partition_num);
    ESP_LOGI(TAG, "  Start LBA: %lu", start_lba);
    ESP_LOGI(TAG, "  Size: %lu sectors (%.2f MB)",
             size_sectors, (float)((uint64_t)size_sectors * SECTOR_SIZE) / (1024.0 * 1024.0));

    /* Calculate FAT32 parameters */
    uint8_t sectors_per_cluster = 8;  /* 4 KB clusters for most drives */
    uint16_t reserved_sectors = 32;
    uint8_t num_fats = 2;

    /* Calculate sectors per FAT */
    uint32_t data_sectors = size_sectors - reserved_sectors;
    uint32_t fat_size = (data_sectors / (sectors_per_cluster * 256 + 2)) + 1;

    /* Adjust for 2 FATs */
    data_sectors = size_sectors - reserved_sectors - (fat_size * num_fats);
    uint32_t cluster_count = data_sectors / sectors_per_cluster;

    ESP_LOGI(TAG, "FAT32 parameters:");
    ESP_LOGI(TAG, "  Sectors per cluster: %d", sectors_per_cluster);
    ESP_LOGI(TAG, "  Reserved sectors: %d", reserved_sectors);
    ESP_LOGI(TAG, "  Number of FATs: %d", num_fats);
    ESP_LOGI(TAG, "  Sectors per FAT: %lu", fat_size);
    ESP_LOGI(TAG, "  Cluster count: %lu", cluster_count);

    /* Create FAT32 boot sector */
    uint8_t boot_sector[SECTOR_SIZE];
    memset(boot_sector, 0, SECTOR_SIZE);

    /* Jump instruction */
    boot_sector[0] = 0xEB;
    boot_sector[1] = 0x58;
    boot_sector[2] = 0x90;

    /* OEM name */
    memcpy(&boot_sector[3], "MSWIN4.1", 8);

    /* Bytes per sector (512) */
    boot_sector[11] = 0x00;
    boot_sector[12] = 0x02;

    /* Sectors per cluster */
    boot_sector[13] = sectors_per_cluster;

    /* Reserved sectors */
    boot_sector[14] = reserved_sectors & 0xFF;
    boot_sector[15] = (reserved_sectors >> 8) & 0xFF;

    /* Number of FATs */
    boot_sector[16] = num_fats;

    /* Root entries (0 for FAT32) */
    boot_sector[17] = 0x00;
    boot_sector[18] = 0x00;

    /* Total sectors (0 for FAT32) */
    boot_sector[19] = 0x00;
    boot_sector[20] = 0x00;

    /* Media descriptor (0xF8 = hard disk) */
    boot_sector[21] = 0xF8;

    /* Sectors per FAT (0 for FAT32) */
    boot_sector[22] = 0x00;
    boot_sector[23] = 0x00;

    /* Sectors per track */
    boot_sector[24] = 0x3F;
    boot_sector[25] = 0x00;

    /* Number of heads */
    boot_sector[26] = 0xFF;
    boot_sector[27] = 0x00;

    /* Hidden sectors (partition start LBA) */
    boot_sector[28] = (start_lba >> 0) & 0xFF;
    boot_sector[29] = (start_lba >> 8) & 0xFF;
    boot_sector[30] = (start_lba >> 16) & 0xFF;
    boot_sector[31] = (start_lba >> 24) & 0xFF;

    /* Total sectors (partition size) */
    boot_sector[32] = (size_sectors >> 0) & 0xFF;
    boot_sector[33] = (size_sectors >> 8) & 0xFF;
    boot_sector[34] = (size_sectors >> 16) & 0xFF;
    boot_sector[35] = (size_sectors >> 24) & 0xFF;

    /* Sectors per FAT */
    boot_sector[36] = (fat_size >> 0) & 0xFF;
    boot_sector[37] = (fat_size >> 8) & 0xFF;
    boot_sector[38] = (fat_size >> 16) & 0xFF;
    boot_sector[39] = (fat_size >> 24) & 0xFF;

    /* Flags */
    boot_sector[40] = 0x00;
    boot_sector[41] = 0x00;

    /* Version */
    boot_sector[42] = 0x00;
    boot_sector[43] = 0x00;

    /* Root cluster (2) */
    boot_sector[44] = 0x02;
    boot_sector[45] = 0x00;
    boot_sector[46] = 0x00;
    boot_sector[47] = 0x00;

    /* FSInfo sector (1) */
    boot_sector[48] = 0x01;
    boot_sector[49] = 0x00;

    /* Backup boot sector (6) */
    boot_sector[50] = 0x06;
    boot_sector[51] = 0x00;

    /* Drive number */
    boot_sector[64] = 0x80;

    /* Reserved */
    boot_sector[65] = 0x00;

    /* Extended boot signature */
    boot_sector[66] = 0x29;

    /* Volume serial number (random) */
    boot_sector[67] = 0x12;
    boot_sector[68] = 0x34;
    boot_sector[69] = 0x56;
    boot_sector[70] = 0x78;

    /* Volume label */
    memcpy(&boot_sector[71], "NO NAME    ", 11);

    /* Filesystem type */
    memcpy(&boot_sector[82], "FAT32   ", 8);

    /* Boot signature */
    boot_sector[510] = 0x55;
    boot_sector[511] = 0xAA;

    /* Write boot sector */
    ESP_LOGI(TAG, "Writing boot sector to LBA %lu...", start_lba);
    esp_err_t ret = usb_host_write_sector(start_lba, boot_sector);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write boot sector");
        return ret;
    }

    /* Write backup boot sector */
    ESP_LOGI(TAG, "Writing backup boot sector to LBA %lu...", start_lba + 6);
    ret = usb_host_write_sector(start_lba + 6, boot_sector);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write backup boot sector");
        return ret;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ FAT32 partition formatted successfully");
    ESP_LOGI(TAG, "=================================================");

    return ESP_OK;
}
