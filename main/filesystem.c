/**
 * @file filesystem.c
 * @brief Internal FATFS Filesystem Implementation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#include "filesystem.h"
#include "esp_log.h"

static const char *TAG = "fs";

esp_err_t filesystem_init(void)
{
    ESP_LOGI(TAG, "Initializing internal FATFS");
    /* TODO: Mount FATFS at /storage */
    return ESP_OK;
}

esp_err_t filesystem_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing FATFS");
    return ESP_OK;
}

esp_err_t filesystem_read_label(char *label_buffer, size_t max_len)
{
    ESP_LOGI(TAG, "Reading label from fatlabel.txt");
    /* TODO: Read label from /storage/fatlabel.txt */
    return ESP_OK;
}

