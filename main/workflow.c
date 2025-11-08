/**
 * @file workflow.c
 * @brief USB Host Workflow Automation Implementation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#include "workflow.h"
#include "usb_host.h"
#include "internal_storage.h"
#include "led_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "workflow";

/* Workflow state */
static workflow_state_t current_state = WORKFLOW_STATE_IDLE;
static workflow_config_t workflow_cfg;
static bool workflow_enabled = false;
static bool workflow_initialized = false;

/* Event group for workflow synchronization */
static EventGroupHandle_t workflow_events = NULL;

/* Event bits */
#define WORKFLOW_EVENT_USB_CONNECTED    (1 << 0)
#define WORKFLOW_EVENT_USB_MOUNTED      (1 << 1)
#define WORKFLOW_EVENT_USB_DISCONNECTED (1 << 2)
#define WORKFLOW_EVENT_TRIGGER          (1 << 3)

/* Task handle */
static TaskHandle_t workflow_task_handle = NULL;

/**
 * @brief Set workflow state and update LED
 */
static void workflow_set_state(workflow_state_t new_state)
{
    if (current_state != new_state) {
        current_state = new_state;
        ESP_LOGI(TAG, "Workflow state: %d", new_state);
        
        /* Update LED based on state */
        switch (new_state) {
            case WORKFLOW_STATE_IDLE:
                led_control_set_state(LED_STATE_IDLE);
                break;
            case WORKFLOW_STATE_PREPARE:
            case WORKFLOW_STATE_FORMAT:
                led_control_set_state(LED_STATE_PREPARE);
                break;
            case WORKFLOW_STATE_COPY:
                led_control_set_state(LED_STATE_COPY);
                break;
            case WORKFLOW_STATE_SYNC:
            case WORKFLOW_STATE_EJECT:
                led_control_set_state(LED_STATE_SYNC);
                break;
            case WORKFLOW_STATE_SUCCESS:
                led_control_set_state(LED_STATE_SUCCESS);
                break;
            case WORKFLOW_STATE_ERROR:
                led_control_set_state(LED_STATE_ERROR);
                break;
        }
    }
}

/**
 * @brief Execute workflow: format -> copy -> eject
 */
static esp_err_t workflow_execute(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Executing Workflow Automation");
    ESP_LOGI(TAG, "=================================================");
    
    esp_err_t ret = ESP_OK;
    
    /* Wait for USB to be mounted (either auto-mounted or auto-formatted) */
    workflow_set_state(WORKFLOW_STATE_PREPARE);
    
    /* Check if USB is mounted */
    if (!usb_host_is_mounted()) {
        ESP_LOGW(TAG, "USB not mounted, waiting...");
        /* Auto-format should have been triggered in usb_host event callback */
        vTaskDelay(pdMS_TO_TICKS(5000));  /* Wait 5s for auto-format */
        
        if (!usb_host_is_mounted()) {
            ESP_LOGE(TAG, "USB still not mounted after 5s");
            workflow_set_state(WORKFLOW_STATE_ERROR);
            vTaskDelay(pdMS_TO_TICKS(3000));
            return ESP_FAIL;
        }
    }
    
    ESP_LOGI(TAG, "✓ USB drive is mounted");
    
    /* Step 1: Copy files from SPIFFS to USB */
    if (workflow_cfg.auto_copy) {
        ESP_LOGI(TAG, "Step 1/2: Copying files from SPIFFS to USB...");
        workflow_set_state(WORKFLOW_STATE_COPY);

        const char *src = internal_storage_get_mount_point();
        const char *dst = usb_host_get_mount_point();

        if (src && dst) {
            uint32_t files_copied = 0;
            ret = usb_host_copy_all_files(src, dst, &files_copied);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "✓ Files copied successfully (%lu files)", files_copied);
            } else {
                ESP_LOGE(TAG, "✗ File copy failed: %s", esp_err_to_name(ret));
                workflow_set_state(WORKFLOW_STATE_ERROR);
                vTaskDelay(pdMS_TO_TICKS(3000));
                return ret;
            }
        } else {
            ESP_LOGE(TAG, "✗ Mount points not available");
            workflow_set_state(WORKFLOW_STATE_ERROR);
            vTaskDelay(pdMS_TO_TICKS(3000));
            return ESP_FAIL;
        }
    }
    
    /* Step 2: Safe eject */
    if (workflow_cfg.auto_eject) {
        ESP_LOGI(TAG, "Step 2/2: Ejecting USB drive...");
        workflow_set_state(WORKFLOW_STATE_EJECT);
        
        ret = usb_host_safe_eject();
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ USB drive ejected successfully");
        } else {
            ESP_LOGE(TAG, "✗ Safe eject failed: %s", esp_err_to_name(ret));
            workflow_set_state(WORKFLOW_STATE_ERROR);
            vTaskDelay(pdMS_TO_TICKS(3000));
            return ret;
        }
    }
    
    /* Success */
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ Workflow Complete!");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "You can now unplug the USB drive");
    
    workflow_set_state(WORKFLOW_STATE_SUCCESS);
    vTaskDelay(pdMS_TO_TICKS(2000));  /* Show success LED for 2s */
    
    /* Return to idle if looping */
    if (workflow_cfg.loop_enabled) {
        ESP_LOGI(TAG, "Waiting for next USB drive...");
        workflow_set_state(WORKFLOW_STATE_IDLE);
    }
    
    return ESP_OK;
}

/**
 * @brief Workflow task - monitors USB events and executes workflow
 */
static void workflow_task(void *arg)
{
    ESP_LOGI(TAG, "Workflow task started");
    
    while (workflow_enabled) {
        /* Wait for USB mounted event or manual trigger */
        EventBits_t bits = xEventGroupWaitBits(
            workflow_events,
            WORKFLOW_EVENT_USB_MOUNTED | WORKFLOW_EVENT_TRIGGER,
            pdTRUE,  /* Clear bits on exit */
            pdFALSE, /* Wait for any bit */
            portMAX_DELAY
        );
        
        if (!workflow_enabled) {
            break;
        }
        
        if (bits & (WORKFLOW_EVENT_USB_MOUNTED | WORKFLOW_EVENT_TRIGGER)) {
            ESP_LOGI(TAG, "USB mounted event received, starting workflow...");
            
            /* Execute workflow */
            workflow_execute();
        }
    }
    
    ESP_LOGI(TAG, "Workflow task stopped");
    workflow_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief Initialize workflow automation
 */
esp_err_t workflow_init(const workflow_config_t *config)
{
    if (workflow_initialized) {
        ESP_LOGW(TAG, "Workflow already initialized");
        return ESP_OK;
    }
    
    if (!config) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing workflow automation...");
    
    /* Copy configuration */
    workflow_cfg = *config;
    
    /* Create event group */
    workflow_events = xEventGroupCreate();
    if (!workflow_events) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }
    
    /* Set initial state */
    workflow_set_state(WORKFLOW_STATE_IDLE);
    
    workflow_initialized = true;
    workflow_enabled = true;
    
    /* Create workflow task */
    BaseType_t ret = xTaskCreate(
        workflow_task,
        "workflow",
        4096,
        NULL,
        5,
        &workflow_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create workflow task");
        vEventGroupDelete(workflow_events);
        workflow_events = NULL;
        workflow_initialized = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ Workflow automation initialized");
    ESP_LOGI(TAG, "  Auto-format: %s", config->auto_format ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  Auto-copy: %s", config->auto_copy ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  Auto-eject: %s", config->auto_eject ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  Loop: %s", config->loop_enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

/**
 * @brief Deinitialize workflow automation
 */
esp_err_t workflow_deinit(void)
{
    if (!workflow_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing workflow automation...");
    
    workflow_enabled = false;
    
    /* Wake up task to exit */
    if (workflow_events) {
        xEventGroupSetBits(workflow_events, WORKFLOW_EVENT_TRIGGER);
    }
    
    /* Wait for task to finish */
    if (workflow_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    /* Delete event group */
    if (workflow_events) {
        vEventGroupDelete(workflow_events);
        workflow_events = NULL;
    }
    
    workflow_initialized = false;
    workflow_set_state(WORKFLOW_STATE_IDLE);
    
    ESP_LOGI(TAG, "✓ Workflow automation deinitialized");
    
    return ESP_OK;
}

/**
 * @brief Get current workflow state
 */
workflow_state_t workflow_get_state(void)
{
    return current_state;
}

/**
 * @brief Enable/disable workflow automation
 */
void workflow_set_enabled(bool enabled)
{
    workflow_enabled = enabled;
    ESP_LOGI(TAG, "Workflow automation %s", enabled ? "enabled" : "disabled");
}

/**
 * @brief Check if workflow is enabled
 */
bool workflow_is_enabled(void)
{
    return workflow_enabled;
}

/**
 * @brief Trigger workflow manually
 */
esp_err_t workflow_trigger(void)
{
    if (!workflow_initialized || !workflow_enabled) {
        ESP_LOGW(TAG, "Workflow not initialized or disabled");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Manual workflow trigger");
    xEventGroupSetBits(workflow_events, WORKFLOW_EVENT_TRIGGER);
    
    return ESP_OK;
}

/**
 * @brief Notify workflow of USB mounted event (called from usb_host)
 */
void workflow_notify_usb_mounted(void)
{
    if (workflow_initialized && workflow_enabled && workflow_events) {
        ESP_LOGI(TAG, "USB mounted notification received");
        xEventGroupSetBits(workflow_events, WORKFLOW_EVENT_USB_MOUNTED);
    }
}

