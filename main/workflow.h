/**
 * @file workflow.h
 * @brief USB Host Workflow Automation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Workflow states
 */
typedef enum {
    WORKFLOW_STATE_IDLE = 0,        /**< Waiting for USB device */
    WORKFLOW_STATE_PREPARE,         /**< USB detected, preparing */
    WORKFLOW_STATE_FORMAT,          /**< Formatting USB drive */
    WORKFLOW_STATE_COPY,            /**< Copying files */
    WORKFLOW_STATE_SYNC,            /**< Syncing filesystem */
    WORKFLOW_STATE_EJECT,           /**< Ejecting USB drive */
    WORKFLOW_STATE_SUCCESS,         /**< Operation complete */
    WORKFLOW_STATE_ERROR,           /**< Error occurred */
} workflow_state_t;

/**
 * @brief Workflow configuration
 */
typedef struct {
    bool auto_format;               /**< Auto-format on mount failure */
    bool auto_copy;                 /**< Auto-copy files after mount */
    bool auto_eject;                /**< Auto-eject after copy */
    bool loop_enabled;              /**< Loop workflow for multiple USB drives */
} workflow_config_t;

/**
 * @brief Initialize workflow automation
 * @param config Workflow configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t workflow_init(const workflow_config_t *config);

/**
 * @brief Deinitialize workflow automation
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t workflow_deinit(void);

/**
 * @brief Get current workflow state
 * @return Current workflow state
 */
workflow_state_t workflow_get_state(void);

/**
 * @brief Enable/disable workflow automation
 * @param enabled true to enable, false to disable
 */
void workflow_set_enabled(bool enabled);

/**
 * @brief Check if workflow is enabled
 * @return true if enabled, false otherwise
 */
bool workflow_is_enabled(void);

/**
 * @brief Trigger workflow manually (for testing)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t workflow_trigger(void);

/**
 * @brief Notify workflow of USB mounted event (called from usb_host)
 */
void workflow_notify_usb_mounted(void);

