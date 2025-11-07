/**
 * @file usb_host.c
 * @brief USB Host Mode Implementation using ESP-IDF USB Host Library
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 2.0.0
 */

#include "usb_host.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb/usb_host.h"
#include "led_control.h"

static const char *TAG = "usb_host";

/* USB Host state */
static bool usb_host_initialized = false;
static bool usb_device_connected = false;
static usb_host_client_handle_t client_hdl = NULL;
static usb_device_handle_t dev_hdl = NULL;
static uint8_t dev_addr = 0;

/* Forward declarations */
static void usb_host_lib_task(void *arg);
static void usb_host_client_task(void *arg);
static void usb_host_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg);

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
            } else {
                ESP_LOGE(TAG, "Failed to open device: %s", esp_err_to_name(ret));
            }
            break;

        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            ESP_LOGI(TAG, "USB device disconnected (handle: %p)", event_msg->dev_gone.dev_hdl);

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

    /* Install USB Host library */
    ESP_LOGI(TAG, "Installing USB Host library...");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Host library: %s", esp_err_to_name(ret));
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

    /* Uninstall USB Host library */
    esp_err_t ret = usb_host_uninstall();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall USB Host library: %s", esp_err_to_name(ret));
        return ret;
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



