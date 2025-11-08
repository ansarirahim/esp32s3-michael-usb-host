/**
 * @file internal_storage.c
 * @brief Internal Storage (SPIFFS) Implementation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#include "internal_storage.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *TAG = "internal_storage";

#define SPIFFS_MOUNT_POINT "/spiffs"
#define SPIFFS_PARTITION_LABEL "storage"

/* Internal storage state */
static bool spiffs_mounted = false;

/**
 * @brief Initialize internal storage (SPIFFS)
 */
esp_err_t internal_storage_init(void)
{
    if (spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_MOUNT_POINT,
        .partition_label = SPIFFS_PARTITION_LABEL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    /* Get partition info */
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(SPIFFS_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition info (%s)", esp_err_to_name(ret));
        esp_vfs_spiffs_unregister(SPIFFS_PARTITION_LABEL);
        return ret;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ SPIFFS mounted successfully");
    ESP_LOGI(TAG, "  Mount point: %s", SPIFFS_MOUNT_POINT);
    ESP_LOGI(TAG, "  Partition: %s", SPIFFS_PARTITION_LABEL);
    ESP_LOGI(TAG, "  Total: %d bytes", total);
    ESP_LOGI(TAG, "  Used: %d bytes", used);
    ESP_LOGI(TAG, "  Free: %d bytes", total - used);
    ESP_LOGI(TAG, "=================================================");

    spiffs_mounted = true;
    return ESP_OK;
}

/**
 * @brief Deinitialize internal storage
 */
esp_err_t internal_storage_deinit(void)
{
    if (!spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS not mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Unmounting SPIFFS...");
    esp_err_t ret = esp_vfs_spiffs_unregister(SPIFFS_PARTITION_LABEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SPIFFS (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "✓ SPIFFS unmounted successfully");
    spiffs_mounted = false;
    return ESP_OK;
}

/**
 * @brief Create sample files in internal storage
 */
esp_err_t internal_storage_create_samples(void)
{
    if (!spiffs_mounted) {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Creating sample files in SPIFFS...");
    ESP_LOGI(TAG, "=================================================");

    /* File 1: README.TXT */
    const char* readme_content = 
        "ESP32-S3 USB Host Automator\n"
        "============================\n"
        "\n"
        "This USB drive was automatically formatted and populated by an ESP32-S3.\n"
        "\n"
        "Project: USB Host Mode Automation\n"
        "Author: Abdul Raheem Ansari\n"
        "Date: November 2025\n"
        "\n"
        "Files on this drive were copied from internal flash storage.\n";

    FILE* f = fopen(SPIFFS_MOUNT_POINT "/README.TXT", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create README.TXT");
        return ESP_FAIL;
    }
    fprintf(f, "%s", readme_content);
    fclose(f);
    ESP_LOGI(TAG, "✓ Created README.TXT (%d bytes)", strlen(readme_content));

    /* File 2: CONFIG.INI */
    const char* config_content =
        "[Settings]\n"
        "AutoFormat=true\n"
        "FileSystem=FAT32\n"
        "ClusterSize=4096\n"
        "VolumeLabel=ESP32-USB\n"
        "\n"
        "[Files]\n"
        "CopyAll=true\n"
        "VerifyAfterCopy=true\n";

    f = fopen(SPIFFS_MOUNT_POINT "/CONFIG.INI", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create CONFIG.INI");
        return ESP_FAIL;
    }
    fprintf(f, "%s", config_content);
    fclose(f);
    ESP_LOGI(TAG, "✓ Created CONFIG.INI (%d bytes)", strlen(config_content));

    /* File 3: DATA.BIN (1024 bytes with pattern) */
    f = fopen(SPIFFS_MOUNT_POINT "/DATA.BIN", "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create DATA.BIN");
        return ESP_FAIL;
    }
    uint8_t pattern[256];
    for (int i = 0; i < 256; i++) {
        pattern[i] = i;
    }
    for (int i = 0; i < 4; i++) {
        fwrite(pattern, 1, 256, f);
    }
    fclose(f);
    ESP_LOGI(TAG, "✓ Created DATA.BIN (1024 bytes)");

    /* File 4: LOG.TXT */
    const char* log_content =
        "ESP32-S3 USB Host Automator - Operation Log\n"
        "============================================\n"
        "\n"
        "[2025-11-08 12:00:00] System initialized\n"
        "[2025-11-08 12:00:01] USB drive detected\n"
        "[2025-11-08 12:00:02] Partition table detected: MBR\n"
        "[2025-11-08 12:00:03] Partition deleted\n"
        "[2025-11-08 12:00:04] New partition created\n"
        "[2025-11-08 12:00:05] Partition formatted: FAT32\n"
        "[2025-11-08 12:00:06] Files copied: 4\n"
        "[2025-11-08 12:00:07] Operation complete\n";

    f = fopen(SPIFFS_MOUNT_POINT "/LOG.TXT", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create LOG.TXT");
        return ESP_FAIL;
    }
    fprintf(f, "%s", log_content);
    fclose(f);
    ESP_LOGI(TAG, "✓ Created LOG.TXT (%d bytes)", strlen(log_content));

    /* File 5: TEST.DAT (4096 bytes with incrementing pattern) */
    f = fopen(SPIFFS_MOUNT_POINT "/TEST.DAT", "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create TEST.DAT");
        return ESP_FAIL;
    }
    for (int i = 0; i < 4096; i++) {
        uint8_t byte = i & 0xFF;
        fwrite(&byte, 1, 1, f);
    }
    fclose(f);
    ESP_LOGI(TAG, "✓ Created TEST.DAT (4096 bytes)");

    /* File 6: fatlabel.txt (FAT volume label configuration) */
    const char* label_content = "MICHAEL-USB";  /* 11 chars max for FAT label */

    f = fopen(SPIFFS_MOUNT_POINT "/fatlabel.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create fatlabel.txt");
        return ESP_FAIL;
    }
    fprintf(f, "%s", label_content);
    fclose(f);
    ESP_LOGI(TAG, "✓ Created fatlabel.txt (%d bytes) - Label: '%s'", strlen(label_content), label_content);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "✓ All sample files created successfully");
    ESP_LOGI(TAG, "=================================================");

    return ESP_OK;
}

/**
 * @brief Get internal storage mount point
 */
const char* internal_storage_get_mount_point(void)
{
    return spiffs_mounted ? SPIFFS_MOUNT_POINT : NULL;
}

/**
 * @brief Check if internal storage is mounted
 */
bool internal_storage_is_mounted(void)
{
    return spiffs_mounted;
}

/**
 * @brief List all files in internal storage
 */
esp_err_t internal_storage_list_files(uint32_t* file_count)
{
    if (!spiffs_mounted) {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Listing files in SPIFFS...");
    ESP_LOGI(TAG, "=================================================");

    DIR* dir = opendir(SPIFFS_MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open SPIFFS directory");
        return ESP_FAIL;
    }

    struct dirent* entry;
    uint32_t count = 0;
    uint32_t total_size = 0;

    while ((entry = readdir(dir)) != NULL) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", SPIFFS_MOUNT_POINT, entry->d_name);

        struct stat st;
        if (stat(filepath, &st) == 0) {
            ESP_LOGI(TAG, "[FILE] %s (%ld bytes)", entry->d_name, st.st_size);
            count++;
            total_size += st.st_size;
        }
    }

    closedir(dir);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Total files: %lu", count);
    ESP_LOGI(TAG, "Total size: %lu bytes", total_size);
    ESP_LOGI(TAG, "=================================================");

    if (file_count != NULL) {
        *file_count = count;
    }

    return ESP_OK;
}

/**
 * @brief Read FAT volume label from fatlabel.txt
 */
esp_err_t internal_storage_read_label(char* label_buffer, size_t buffer_size)
{
    if (!spiffs_mounted) {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    if (!label_buffer || buffer_size < 12) {  /* FAT label is 11 chars + null */
        ESP_LOGE(TAG, "Invalid parameters (buffer must be at least 12 bytes)");
        return ESP_ERR_INVALID_ARG;
    }

    /* Open fatlabel.txt */
    FILE* f = fopen(SPIFFS_MOUNT_POINT "/fatlabel.txt", "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "fatlabel.txt not found, using default label");
        /* Use default label */
        snprintf(label_buffer, buffer_size, "ESP32S3");
        return ESP_OK;
    }

    /* Read label (max 11 characters for FAT) */
    char temp_buffer[32];
    if (fgets(temp_buffer, sizeof(temp_buffer), f) == NULL) {
        ESP_LOGE(TAG, "Failed to read fatlabel.txt");
        fclose(f);
        snprintf(label_buffer, buffer_size, "ESP32S3");
        return ESP_FAIL;
    }
    fclose(f);

    /* Remove newline if present */
    size_t len = strlen(temp_buffer);
    if (len > 0 && temp_buffer[len - 1] == '\n') {
        temp_buffer[len - 1] = '\0';
        len--;
    }
    if (len > 0 && temp_buffer[len - 1] == '\r') {
        temp_buffer[len - 1] = '\0';
        len--;
    }

    /* Validate label length (FAT labels are max 11 characters) */
    if (len > 11) {
        ESP_LOGW(TAG, "Label too long (%d chars), truncating to 11", len);
        temp_buffer[11] = '\0';
        len = 11;
    }

    /* Copy to output buffer */
    snprintf(label_buffer, buffer_size, "%s", temp_buffer);

    ESP_LOGI(TAG, "✓ Read FAT label from fatlabel.txt: '%s' (%d chars)", label_buffer, len);
    return ESP_OK;
}
