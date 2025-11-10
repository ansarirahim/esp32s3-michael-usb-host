/*
 * FatFS Storage for LPC21ISP ESP32
 * 
 * This file provides FatFS filesystem operations for internal flash storage
 */

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_partition.h"
#include "wear_levelling.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static const char *TAG = "fatfs_lpc21isp";

// Mount point for FatFS (must match MSC mode mount path for shared filesystem)
#define BASE_PATH "/data"
#define PARTITION_LABEL "storage"

// Global wear levelling handle
static wl_handle_t wl_handle = WL_INVALID_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize FatFS on internal flash partition
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fatfs_init(void)
{
    ESP_LOGI(TAG, "Initializing FatFS on partition '%s'", PARTITION_LABEL);
    
    // Find the partition
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_FAT,
        PARTITION_LABEL
    );
    
    if (partition == NULL) {
        ESP_LOGW(TAG, "FatFS partition '%s' not found - filesystem operations will be unavailable", PARTITION_LABEL);
        ESP_LOGW(TAG, "To enable FatFS, rebuild and reflash with the partition table that includes the 'storage' partition");
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Found partition: %s, size: %d bytes, address: 0x%x", 
             partition->label, partition->size, partition->address);
    
    // Wear levelling configuration
    // Note: Field order must match struct declaration order
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,  // Format if mount fails
        .max_files = 5,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .disk_status_check_enable = false
    };
    
    // Mount FatFS (base_path and partition_label are separate parameters)
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(
        BASE_PATH,
        PARTITION_LABEL,
        &mount_config,
        &wl_handle
    );
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find FATFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize FATFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    ESP_LOGI(TAG, "FatFS mounted successfully at %s", BASE_PATH);
    return ESP_OK;
}

/**
 * @brief Unmount FatFS
 */
void fatfs_unmount(void)
{
    ESP_LOGI(TAG, "Unmounting FatFS");
    esp_vfs_fat_spiflash_unmount_rw_wl(BASE_PATH, wl_handle);
}

/**
 * @brief Write a file to FatFS
 * 
 * @param filename File name
 * @param content Content to write
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fatfs_write_file(const char *filename, const char *content)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_PATH, filename);
    
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        return ESP_FAIL;
    }
    
    fprintf(f, "%s", content);
    fclose(f);
    
    ESP_LOGI(TAG, "File written: %s", filepath);
    return ESP_OK;
}

/**
 * @brief Read binary file from FatFS (for hex files)
 * 
 * @param filename File name
 * @param buffer Buffer to store content (will be allocated)
 * @param file_size Output parameter for file size
 * @return esp_err_t ESP_OK on success, caller must free buffer
 */
esp_err_t fatfs_read_binary_file(const char *filename, uint8_t **buffer, size_t *file_size)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_PATH, filename);
    
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filepath);
        return ESP_FAIL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(f);
        ESP_LOGE(TAG, "Invalid file size: %ld", size);
        return ESP_FAIL;
    }
    
    // Allocate buffer
    *buffer = (uint8_t *)malloc(size);
    if (*buffer == NULL) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to allocate memory for file");
        return ESP_ERR_NO_MEM;
    }
    
    // Read file
    size_t read_len = fread(*buffer, 1, size, f);
    fclose(f);
    
    if (read_len != (size_t)size) {
        free(*buffer);
        *buffer = NULL;
        ESP_LOGE(TAG, "Failed to read complete file: read %d of %ld bytes", read_len, size);
        return ESP_FAIL;
    }
    
    *file_size = read_len;
    ESP_LOGI(TAG, "Binary file read: %s (%d bytes)", filepath, read_len);
    return ESP_OK;
}

/**
 * @brief Read a file from FatFS
 * 
 * @param filename File name
 * @param buffer Buffer to store content
 * @param max_len Maximum length to read
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fatfs_read_file(const char *filename, char *buffer, size_t max_len)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_PATH, filename);
    
    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filepath);
        return ESP_FAIL;
    }
    
    size_t read_len = fread(buffer, 1, max_len - 1, f);
    fclose(f);
    
    buffer[read_len] = '\0';
    ESP_LOGI(TAG, "File read: %s (%d bytes)", filepath, read_len);
    return ESP_OK;
}

/**
 * @brief Check if a file exists in FatFS
 * 
 * @param filename File name
 * @return true if file exists, false otherwise
 */
bool fatfs_file_exists(const char *filename)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_PATH, filename);
    
    FILE *f = fopen(filepath, "r");
    if (f != NULL) {
        fclose(f);
        return true;
    }
    return false;
}

/**
 * @brief Copy binary file to FatFS
 * 
 * @param filename File name
 * @param data Binary data to write
 * @param data_len Length of binary data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fatfs_write_binary_file(const char *filename, const uint8_t *data, size_t data_len)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_PATH, filename);
    
    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        return ESP_FAIL;
    }
    
    size_t written = fwrite(data, 1, data_len, f);
    fclose(f);
    
    if (written != data_len) {
        ESP_LOGE(TAG, "Failed to write complete file: %s (wrote %d of %d bytes)", filepath, written, data_len);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Binary file written: %s (%d bytes)", filepath, data_len);
    return ESP_OK;
}

/**
 * @brief List files in FatFS directory
 */
void fatfs_list_files(void)
{
    DIR *dir = opendir(BASE_PATH);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", BASE_PATH);
        return;
    }
    
    ESP_LOGI(TAG, "Files in %s:", BASE_PATH);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        ESP_LOGI(TAG, "  %s", entry->d_name);
    }
    closedir(dir);
}

#ifdef __cplusplus
}
#endif

