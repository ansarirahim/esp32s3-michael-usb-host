/**
 * @file internal_storage.h
 * @brief Internal Storage (SPIFFS) Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize internal storage (SPIFFS)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_init(void);

/**
 * @brief Deinitialize internal storage
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_deinit(void);

/**
 * @brief Create sample files in internal storage
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_create_samples(void);

/**
 * @brief Get internal storage mount point
 * @return Mount point string or NULL if not mounted
 */
const char* internal_storage_get_mount_point(void);

/**
 * @brief Check if internal storage is mounted
 * @return true if mounted, false otherwise
 */
bool internal_storage_is_mounted(void);

/**
 * @brief List all files in internal storage
 * @param file_count Pointer to store file count (optional)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_list_files(uint32_t* file_count);

/**
 * @brief Read FAT volume label from fatlabel.txt
 * @param label_buffer Buffer to store the label (must be at least 12 bytes)
 * @param buffer_size Size of the label buffer
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_read_label(char* label_buffer, size_t buffer_size);
