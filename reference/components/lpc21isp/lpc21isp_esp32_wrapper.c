/*
 * ESP32 Wrapper Implementation for lpc21isp
 */

#include "lpc21isp_esp32.h"
#include "esp32_serial_adapter.h"
// lpc21isp.h is already included via lpc21isp_esp32.h
#include "lpcprog.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

// Forward declaration for FatFS function
extern esp_err_t fatfs_read_binary_file(const char *filename, uint8_t **buffer, size_t *file_size);

static const char *TAG = "lpc21isp_wrapper";
static volatile bool s_synchronized_flag = false;  // Flag to track bootloader sync state

ISP_ENVIRONMENT* lpc21isp_esp32_init(cdc_acm_dev_hdl_t cdc_dev, const char *baud_rate)
{
    if (cdc_dev == NULL || baud_rate == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return NULL;
    }
    
    // Set the CDC device in the adapter
    esp32_serial_adapter_set_device(cdc_dev);
    
    // Allocate and initialize ISP environment from PSRAM (with fallback to regular heap)
    ISP_ENVIRONMENT *env = (ISP_ENVIRONMENT *)heap_caps_calloc(1, sizeof(ISP_ENVIRONMENT), MALLOC_CAP_SPIRAM);
    if (env == NULL) {
        // Fallback to regular heap if PSRAM is not available
        ESP_LOGW(TAG, "PSRAM allocation failed, falling back to regular heap");
        env = (ISP_ENVIRONMENT *)calloc(1, sizeof(ISP_ENVIRONMENT));
        if (env == NULL) {
            ESP_LOGE(TAG, "Failed to allocate ISP environment from heap");
            return NULL;
        }
    } else {
        ESP_LOGI(TAG, "ISP environment allocated from PSRAM");
    }
    
    // Initialize default values
    env->micro = NXP_ARM;
    env->FileFormat = FORMAT_BINARY;
    env->ProgramChip = 1;
    env->ControlLines = 0;  // Disable control line handling - we handle reset manually
    env->ControlLinesSwapped = 0;
    env->ControlLinesInverted = 0;
    env->LogFile = 0;
    env->f_list = NULL;
    env->nQuestionMarks = 100;
    env->DoNotStart = 0;
    env->BootHold = 0;
    env->serial_port = NULL;  // Not used for ESP32
    env->TerminalOnly = 0;
    env->HalfDuplex = 0;
    env->WriteDelay = 0;
    env->DetectOnly = 0;
    env->WipeDevice = 0;
    env->Verify = 0;
    env->DetectedDevice = 0;
    env->baud_rate = strdup(baud_rate);
    strcpy(env->StringOscillator, "12000");
    env->FileContent = NULL;
    env->BinaryContent = NULL;
    env->BinaryLength = 0;
    env->BinaryOffset = 0;
    env->StartAddress = 0;
    env->BinaryMemSize = 0;
    env->serial_timeout_count = 0;
    
    // Open serial port (ESP32 adapter)
    OpenSerialPort(env);
    
    ESP_LOGI(TAG, "lpc21isp initialized with baud rate: %s", baud_rate);
    
    return env;
}

int lpc21isp_esp32_program(ISP_ENVIRONMENT *env, 
                           const uint8_t *firmware_data, 
                           size_t firmware_size,
                           unsigned long start_address)
{
    if (env == NULL || firmware_data == NULL || firmware_size == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }
    
    // Set up binary content
    env->BinaryContent = (BINARY *)firmware_data;
    env->BinaryLength = firmware_size;
    env->BinaryOffset = 0;
    env->StartAddress = start_address;
    
    // Set line coding (baud rate) - this should be done before programming
    cdc_acm_line_coding_t line_coding = {
        .dwDTERate = (uint32_t)atol(env->baud_rate),
        .bCharFormat = 0,  // 1 stop bit
        .bParityType = 0,  // None
        .bDataBits = 8,
    };
    
    cdc_acm_dev_hdl_t cdc_dev = NULL;
    esp32_serial_adapter_get_device(&cdc_dev);
    if (cdc_dev) {
        esp_err_t ret = cdc_acm_host_line_coding_set(cdc_dev, &line_coding);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set line coding: %s", esp_err_to_name(ret));
        }
    }
    
    // Call the programming function
    int result = NxpDownload(env);
    
    if (result == 0) {
        ESP_LOGI(TAG, "Programming completed successfully");
    } else {
        ESP_LOGE(TAG, "Programming failed with error code: 0x%X", result);
    }
    
    return result;
}

bool lpc21isp_esp32_is_synchronized(void)
{
    return s_synchronized_flag;
}

void lpc21isp_esp32_reset_sync_flag(void)
{
    s_synchronized_flag = false;
}

void lpc21isp_esp32_set_synchronized(void)
{
    s_synchronized_flag = true;
}

int lpc21isp_esp32_sync_bootloader(ISP_ENVIRONMENT *env, int num_attempts, bool check_response)
{
    // NOTE: This function is deprecated - sync attempts are now handled by the internal
    // AppSyncing function in lpcprog.c. This function is kept for backward compatibility
    // but should not be used. The internal sync in AppSyncing handles all sync attempts.
    
    if (env == NULL) {
        ESP_LOGE(TAG, "Invalid ISP environment");
        return -1;
    }
    
    // Reset sync flag at start
    s_synchronized_flag = false;
    
    // Use fixed 5 attempts with 1 second delay
    // NOTE: Internal sync (AppSyncing) handles the actual sync attempts
    const int sync_attempts = 5;
    const int sync_delay_ms = 1000; // 1 second delay
    
    ESP_LOGI(TAG, "Sending '?' sync characters (attempts: %d, delay: %dms)", sync_attempts, sync_delay_ms);
    
    // Clear any pending data
    ClearSerialPortBuffers(env);
    
    // Send '?' characters until synchronized
    for (int i = 0; i < sync_attempts; i++) {
        // Ensure RTS stays high before each sync attempt
        cdc_acm_dev_hdl_t cdc_dev = NULL;
        esp32_serial_adapter_get_device(&cdc_dev);
        if (cdc_dev) {
            cdc_acm_host_set_control_line_state(cdc_dev, false, true);  // DTR=unused, RTS=high
        }
        
        SendComPort(env, "?");
        ESP_LOGI(TAG, "Sent '?' sync character %d/%d", i + 1, sync_attempts);
        
        if (check_response) {
            // Wait a bit for response, then check both the callback flag and try to read
            vTaskDelay(pdMS_TO_TICKS(200)); // Wait for callback to process
            
            // Check if callback detected "Synchronized"
            if (s_synchronized_flag) {
                ESP_LOGI(TAG, "Bootloader synchronized after %d attempts (detected by callback) - stopping sync", i + 1);
                return 0; // Exit immediately, don't send more '?'
            }
            
            // Also try to read directly (in case callback hasn't processed yet)
            char answer[256];
            unsigned long real_size = 0;
            ReceiveComPort(env, answer, sizeof(answer) - 1, &real_size, 1, 300);
            
            if (real_size > 0) {
                answer[real_size] = '\0';
                
                // Check for "Synchronized" response (case-insensitive check)
                char *str_lower = (char *)malloc(real_size + 1);
                if (str_lower != NULL) {
                    // Convert to lowercase for comparison
                    for (unsigned long j = 0; j < real_size; j++) {
                        str_lower[j] = tolower((unsigned char)answer[j]);
                    }
                    str_lower[real_size] = '\0';
                    
                    if (strstr(str_lower, "synchronized") != NULL) {
                        s_synchronized_flag = true;
                        ESP_LOGI(TAG, "Bootloader synchronized after %d attempts (detected by read) - stopping sync", i + 1);
                        free(str_lower);
                        return 0; // Exit immediately, don't send more '?'
                    }
                    free(str_lower);
                } else {
                    // Fallback if malloc fails
                    if (strstr(answer, "Synchronized") != NULL || strstr(answer, "synchronized") != NULL) {
                        s_synchronized_flag = true;
                        ESP_LOGI(TAG, "Bootloader synchronized after %d attempts - stopping sync", i + 1);
                        return 0; // Exit immediately, don't send more '?'
                    }
                }
                
                ESP_LOGD(TAG, "Response: %.*s", (int)real_size, answer);
            }
            
            // Check flag again after read attempt
            if (s_synchronized_flag) {
                ESP_LOGI(TAG, "Bootloader synchronized after %d attempts - stopping sync", i + 1);
                return 0;
            }
            
            // Only delay if we haven't synchronized yet
            if (i < sync_attempts - 1) { // Don't delay after last attempt
                vTaskDelay(pdMS_TO_TICKS(sync_delay_ms - 200)); // Remaining delay (200ms already waited)
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(sync_delay_ms)); // Full delay if not checking response
        }
    }
    
    if (check_response) {
        ESP_LOGW(TAG, "No 'Synchronized' response received after %d attempts", sync_attempts);
        return -1;
    }
    
    ESP_LOGI(TAG, "Sent %d '?' sync characters", sync_attempts);
    return 0;
}

void lpc21isp_esp32_reset_to_bootloader(ISP_ENVIRONMENT *env)
{
    if (env == NULL) {
        ESP_LOGE(TAG, "Invalid ISP environment");
        return;
    }
    
    ESP_LOGI(TAG, "Resetting target to bootloader mode");
    ResetTarget(env, PROGRAM_MODE);
}

void lpc21isp_esp32_cleanup(ISP_ENVIRONMENT *env)
{
    if (env == NULL) {
        return;
    }
    
    CloseSerialPort(env);
    
    if (env->baud_rate) {
        free(env->baud_rate);
    }
    
    // Free binary content if allocated (from PSRAM)
    if (env->BinaryContent != NULL) {
        heap_caps_free(env->BinaryContent);
        env->BinaryContent = NULL;
        env->BinaryMemSize = 0;
    }
    
    if (env->f_list) {
        // Free file list if needed
        FILE_LIST *curr = env->f_list;
        while (curr) {
            FILE_LIST *next = curr->prev;
            free(curr);
            curr = next;
        }
    }
    
    // Free environment (heap_caps_free handles both PSRAM and regular heap)
    heap_caps_free(env);
    ESP_LOGI(TAG, "lpc21isp cleanup completed");
}

// Helper function to convert hex char to value
static unsigned char hex_char_to_value(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return (unsigned char)(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (unsigned char)(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return (unsigned char)(c - 'a' + 10);
    }
    return 0;
}

int lpc21isp_esp32_program_from_hex_file(ISP_ENVIRONMENT *env, const char *hex_filename)
{
    if (env == NULL || hex_filename == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }
    
    ESP_LOGI(TAG, "Reading hex file from storage: %s", hex_filename);
    
    // Read hex file from FatFS
    uint8_t *hex_data = NULL;
    size_t hex_file_size = 0;
    esp_err_t ret = fatfs_read_binary_file(hex_filename, &hex_data, &hex_file_size);
    if (ret != ESP_OK || hex_data == NULL) {
        ESP_LOGE(TAG, "Failed to read hex file from storage: %s", hex_filename);
        return -1;
    }
    
    ESP_LOGI(TAG, "Hex file read: %d bytes, parsing...", hex_file_size);
    
    // Check available heap memory
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Available heap: %d bytes", free_heap);
    
    // Free any existing binary content from previous calls to prevent memory leaks
    if (env->BinaryContent != NULL) {
        ESP_LOGI(TAG, "Freeing previous binary content: %lu bytes", env->BinaryMemSize);
        free(env->BinaryContent);
        env->BinaryContent = NULL;
        env->BinaryMemSize = 0;
    }
    
    // Parse Intel HEX format to binary
    // Initialize binary content
    env->BinaryContent = NULL;
    env->BinaryLength = 0;
    env->BinaryOffset = 0;
    env->StartAddress = 0;
    env->BinaryMemSize = 0;
    unsigned long BinaryMemSize = 0;
    
    unsigned long Pos = 0;
    unsigned long RealAddress = 0;
    int BinaryOffsetDefined = 0;
    
    ESP_LOGI(TAG, "Starting hex file parsing...");
    
    while (Pos < hex_file_size) {
        // Skip whitespace
        if (hex_data[Pos] == '\r' || hex_data[Pos] == '\n') {
            Pos++;
            continue;
        }
        
        // Check for start of record
        if (hex_data[Pos] != ':') {
            ESP_LOGE(TAG, "Invalid hex record at position %lu: expected ':', got 0x%02X", Pos, hex_data[Pos]);
            free(hex_data);
            if (env->BinaryContent) {
                heap_caps_free(env->BinaryContent);
                env->BinaryContent = NULL;
            }
            env->BinaryMemSize = 0;
            return -1;
        }
        Pos++;
        
        // Read record length
        unsigned char RecordLength = (hex_char_to_value(hex_data[Pos]) << 4) | hex_char_to_value(hex_data[Pos + 1]);
        Pos += 2;
        
        // Read record address
        unsigned short RecordAddress = 0;
        for (int i = 0; i < 4; i++) {
            RecordAddress = (RecordAddress << 4) | hex_char_to_value(hex_data[Pos++]);
        }
        
        // Update real address
        RealAddress = RealAddress - (RealAddress & 0xffff) + RecordAddress;
        
        // Read record type
        unsigned char RecordType = (hex_char_to_value(hex_data[Pos]) << 4) | hex_char_to_value(hex_data[Pos + 1]);
        Pos += 2;
        
        // Process based on record type
        if (RecordType == 0x00) {  // Data record
            if (!BinaryOffsetDefined) {
                env->BinaryOffset = RealAddress;
                BinaryOffsetDefined = 1;
                ESP_LOGI(TAG, "First data record: RealAddress=0x%08lX, BinaryOffset=0x%08lX", 
                         RealAddress, env->BinaryOffset);
            }
            
            // Calculate required size
            unsigned long required_size = RealAddress + RecordLength - env->BinaryOffset;
            if (required_size > env->BinaryLength) {
                env->BinaryLength = required_size;
            }
            
            // Grow buffer if needed
            if (required_size > env->BinaryMemSize) {
                unsigned long new_size = required_size;
                // Round up to next 4KB boundary for better alignment
                new_size = ((new_size + 4095) / 4096) * 4096;
                
                // Allocate from PSRAM using heap_caps_realloc (with fallback to regular heap)
                BINARY *new_buffer = NULL;
                if (env->BinaryContent == NULL) {
                    // First allocation - try PSRAM first
                    new_buffer = (BINARY *)heap_caps_malloc(new_size, MALLOC_CAP_SPIRAM);
                    if (!new_buffer) {
                        // Fallback to regular heap
                        ESP_LOGW(TAG, "PSRAM allocation failed, using regular heap for binary content");
                        new_buffer = (BINARY *)malloc(new_size);
                    }
                } else {
                    // Reallocation - try PSRAM first
                    new_buffer = (BINARY *)heap_caps_realloc(env->BinaryContent, new_size, MALLOC_CAP_SPIRAM);
                    if (!new_buffer) {
                        // Fallback to regular heap
                        ESP_LOGW(TAG, "PSRAM reallocation failed, using regular heap for binary content");
                        new_buffer = (BINARY *)realloc(env->BinaryContent, new_size);
                    }
                }
                if (!new_buffer) {
                    ESP_LOGE(TAG, "Failed to allocate memory: %lu bytes", new_size);
                    free(hex_data);
                    if (env->BinaryContent) {
                        // Use heap_caps_free which handles both PSRAM and regular heap
                        heap_caps_free(env->BinaryContent);
                        env->BinaryContent = NULL;
                    }
                    env->BinaryMemSize = 0;
                    return -1;
                }
                
                // Initialize new portion with 0xFF
                if (new_size > env->BinaryMemSize) {
                    memset(new_buffer + env->BinaryMemSize, 0xFF, new_size - env->BinaryMemSize);
                }
                
                env->BinaryContent = new_buffer;
                env->BinaryMemSize = new_size;
            }
            
            // Read data bytes
            for (int i = 0; i < RecordLength; i++) {
                unsigned char byte = (hex_char_to_value(hex_data[Pos]) << 4) | hex_char_to_value(hex_data[Pos + 1]);
                Pos += 2;
                env->BinaryContent[RealAddress + i - env->BinaryOffset] = byte;
            }
            
            // Skip checksum (2 hex chars)
            Pos += 2;
            
            // Skip to end of line (newline)
            while (Pos < hex_file_size && hex_data[Pos] != '\n' && hex_data[Pos] != '\r') {
                Pos++;
            }
        } else if (RecordType == 0x01) {  // End of file
            break;
        } else if (RecordType == 0x02) {  // Extended segment address
            unsigned long seg_addr = 0;
            for (int i = 0; i < RecordLength * 2; i++) {
                seg_addr = (seg_addr << 4) | hex_char_to_value(hex_data[Pos++]);
            }
            RealAddress = (seg_addr << 4);
            Pos += 2;  // Skip checksum
        } else if (RecordType == 0x03) {  // Start segment address
            unsigned long StartAddress = 0;
            for (int i = 0; i < RecordLength * 2; i++) {
                StartAddress = (StartAddress << 4) | hex_char_to_value(hex_data[Pos++]);
            }
            unsigned long cs = StartAddress >> 16;
            unsigned long ip = StartAddress & 0xffff;
            env->StartAddress = cs * 16 + ip;
            Pos += 2;  // Skip checksum
        } else if (RecordType == 0x04) {  // Extended linear address
            unsigned long lin_addr = 0;
            for (int i = 0; i < RecordLength * 2; i++) {
                lin_addr = (lin_addr << 4) | hex_char_to_value(hex_data[Pos++]);
            }
            RealAddress = (lin_addr << 16);
            if (!BinaryOffsetDefined) {
                // Set binary offset using LPC_FLASHMASK (0x000FFFFF) to allow memory range
                env->BinaryOffset = RealAddress & 0x000FFFFF;
                BinaryOffsetDefined = 1;
                ESP_LOGI(TAG, "Extended linear address record [04]: RealAddress=0x%08lX, BinaryOffset=0x%08lX", 
                         RealAddress, env->BinaryOffset);
            } else {
                // Check if address is in range
                if ((RealAddress & 0x000FFFFF) != env->BinaryOffset) {
                    ESP_LOGW(TAG, "New Extended Linear Address Record [04] out of memory range: 0x%08lX vs 0x%08lX", 
                             RealAddress & 0x000FFFFF, env->BinaryOffset);
                }
            }
            Pos += 2;  // Skip checksum
        } else if (RecordType == 0x05) {  // Start linear address
            env->StartAddress = 0;
            for (int i = 0; i < RecordLength * 2; i++) {
                env->StartAddress = (env->StartAddress << 4) | hex_char_to_value(hex_data[Pos++]);
            }
            Pos += 2;  // Skip checksum
        } else {
            ESP_LOGW(TAG, "Unsupported hex record type: 0x%02X at position %lu", RecordType, Pos);
            // Skip this record
            Pos += RecordLength * 2 + 2;  // Skip data and checksum
        }
    }
    
    free(hex_data);
    
    // Log parsing result BEFORE calling NxpDownload (which may corrupt memory)
    ESP_LOGI(TAG, "Hex file parsed: binary length=%lu, start address=0x%08lX", 
             env->BinaryLength, env->StartAddress);
    
    if (env->BinaryLength == 0) {
        ESP_LOGE(TAG, "No binary data extracted from hex file");
        if (env->BinaryContent) {
            free(env->BinaryContent);
            env->BinaryContent = NULL;
            env->BinaryMemSize = 0;
        }
        return -1;
    }
    
    // Set file format
    env->FileFormat = FORMAT_HEX;
    
    // Ensure ControlLines is disabled to prevent ResetTarget from being called
    // We've already synchronized the bootloader, so no reset is needed
    env->ControlLines = 0;
    
    // Save pointer to BinaryContent before calling NxpDownload
    // NxpDownload might free or modify it, so we need to track it separately
    void *binary_content_ptr = env->BinaryContent;
    
    // Clear RX buffer before NxpDownload to remove any leftover data from sync
    // The sync process may have left data in the buffer that NxpDownload shouldn't see
    ClearSerialPortBuffers(env);
    vTaskDelay(pdMS_TO_TICKS(50)); // Small delay to ensure buffer is cleared
    
    // Disable DebugPrintf to avoid heap corruption during NxpDownload
    // DebugPrintf uses printf/vprintf which allocate heap memory
    extern int debug_level;
    int saved_debug_level = debug_level;
    debug_level = 99;  // Set very high to disable all DebugPrintf calls
    
    // IMPORTANT: Caller must set programming_in_progress flag BEFORE calling this function
    // to prevent rx_processing_task from logging during NxpDownload
    // Call NxpDownload to program the device
    int result = NxpDownload(env);
    
    // Restore debug level after NxpDownload
    debug_level = saved_debug_level;
    
    // Clean up binary content after programming
    // Free the memory to prevent leaks on subsequent calls
    if (binary_content_ptr != NULL && env != NULL) {
        // Check if the pointer is still valid (might have been freed/modified by NxpDownload)
        if (env->BinaryContent == binary_content_ptr) {
            // Pointer is still the same - safe to free from PSRAM
            ESP_LOGI(TAG, "Freeing binary content after programming: %lu bytes", env->BinaryMemSize);
            heap_caps_free(env->BinaryContent);
            env->BinaryContent = NULL;
            env->BinaryMemSize = 0;
        } else if (env->BinaryContent != NULL) {
            // Pointer changed - NxpDownload may have reallocated or allocated new memory
                    // Free our original pointer (it was allocated by us from PSRAM)
                    ESP_LOGI(TAG, "BinaryContent pointer changed by NxpDownload, freeing original: %lu bytes", env->BinaryMemSize);
                    // Free our original pointer from PSRAM
                    heap_caps_free(binary_content_ptr);
                    // Free the new pointer if NxpDownload allocated it (it's in our env structure, so we own it)
                    // Note: NxpDownload might have allocated from regular heap, but we'll use heap_caps_free which handles both
                    heap_caps_free(env->BinaryContent);
            env->BinaryContent = NULL;
            env->BinaryMemSize = 0;
        } else {
            // Pointer was set to NULL by NxpDownload - already freed, just clear our tracking
            // However, we should still try to free our saved pointer to be safe
            // (it might not have been freed if NxpDownload just set env->BinaryContent to NULL)
            ESP_LOGI(TAG, "BinaryContent set to NULL by NxpDownload, freeing saved pointer: %lu bytes", env->BinaryMemSize);
            heap_caps_free(binary_content_ptr);  // Free from PSRAM
            env->BinaryMemSize = 0;
        }
        // Clear length regardless
        env->BinaryLength = 0;
    }
    
    // Don't log after NxpDownload - it may have corrupted memory including logging system
    // Just return the result - caller can log if needed
    return result;
}

