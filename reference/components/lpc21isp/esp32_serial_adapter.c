/*
 * ESP32 Serial Adapter for lpc21isp
 * 
 * This file provides ESP32 CDC-ACM serial port adapter for lpc21isp
 */

#include "esp32_serial_adapter.h"
#include "lpc21isp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/cdc_acm_host.h"
#include "usb/cdc_acm_host_ops.h"
#include <string.h>

static const char *TAG = "lpc21isp_adapter";
static cdc_acm_dev_hdl_t s_cdc_dev = NULL;
static bool s_rts_keep_high = false;  // Flag to keep RTS high during bootloader operations

// Forward declaration
extern ISP_ENVIRONMENT *g_isp_env;

// Circular buffer for received serial data (used by ReceiveComPortBlock)
#define RX_BUFFER_SIZE 4096
static uint8_t s_rx_buffer[RX_BUFFER_SIZE];
static volatile size_t s_rx_write_pos = 0;
static volatile size_t s_rx_read_pos = 0;
static SemaphoreHandle_t s_rx_buffer_mutex = NULL;

// Initialize the receive buffer (called once)
static void init_rx_buffer(void)
{
    if (s_rx_buffer_mutex == NULL) {
        s_rx_buffer_mutex = xSemaphoreCreateMutex();
        if (s_rx_buffer_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create RX buffer mutex");
        }
    }
    s_rx_write_pos = 0;
    s_rx_read_pos = 0;
}

// Helper function to ensure RTS stays high
static void ensure_rts_high(void)
{
    if (s_cdc_dev != NULL && s_rts_keep_high) {
        const bool dtr = false;  // DTR not connected
        cdc_acm_host_set_control_line_state(s_cdc_dev, dtr, true);  // RTS=high
    }
}

void esp32_serial_adapter_set_device(cdc_acm_dev_hdl_t cdc_dev)
{
    s_cdc_dev = cdc_dev;
}

void esp32_serial_adapter_get_device(cdc_acm_dev_hdl_t *cdc_dev)
{
    if (cdc_dev) {
        *cdc_dev = s_cdc_dev;
    }
}

void OpenSerialPort(ISP_ENVIRONMENT *IspEnvironment)
{
    if (s_cdc_dev == NULL) {
        ESP_LOGE(TAG, "CDC device not set!");
        return;
    }
    
    // Initialize RX buffer
    init_rx_buffer();
    
    // Don't set RTS here - let ResetTarget handle the reset sequence
    // Just ensure we have a valid device handle
    ESP_LOGI(TAG, "Serial port opened (CDC-ACM)");
    
    // Give the device some time to stabilize after opening
    // This helps avoid TX timeout issues
    vTaskDelay(pdMS_TO_TICKS(100));
}

// Add received data to the circular buffer (called from USB callback)
void esp32_serial_adapter_add_rx_data(const uint8_t *data, size_t data_len)
{
    if (data == NULL || data_len == 0 || s_rx_buffer_mutex == NULL) {
        return;
    }
    
    // Debug: Log received data (limit to first 32 bytes to save stack)
    // Use ESP_LOG_BUFFER_HEX_LEVEL for binary data to avoid stack usage
    if (data_len <= 32) {
        ESP_LOGD(TAG, "RX buffer: Adding %zu bytes", data_len);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, data_len, ESP_LOG_DEBUG);
    } else {
        ESP_LOGD(TAG, "RX buffer: Adding %zu bytes (showing first 32)", data_len);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, 32, ESP_LOG_DEBUG);
    }
    
    if (xSemaphoreTake(s_rx_buffer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        size_t bytes_added = 0;
        for (size_t i = 0; i < data_len; i++) {
            size_t next_write_pos = (s_rx_write_pos + 1) % RX_BUFFER_SIZE;
            
            // Check if buffer is full (would overwrite unread data)
            if (next_write_pos == s_rx_read_pos) {
                // Buffer full - drop oldest byte (overwrite)
                ESP_LOGW(TAG, "RX buffer full! Dropping oldest byte");
                s_rx_read_pos = (s_rx_read_pos + 1) % RX_BUFFER_SIZE;
            }
            
            s_rx_buffer[s_rx_write_pos] = data[i];
            s_rx_write_pos = next_write_pos;
            bytes_added++;
        }
        size_t available = (s_rx_write_pos >= s_rx_read_pos) ? 
                          (s_rx_write_pos - s_rx_read_pos) : 
                          (RX_BUFFER_SIZE - s_rx_read_pos + s_rx_write_pos);
        xSemaphoreGive(s_rx_buffer_mutex);
        ESP_LOGD(TAG, "RX buffer: Added %zu bytes, buffer now has %zu bytes available", 
                 bytes_added, available);
    }
}

void CloseSerialPort(ISP_ENVIRONMENT *IspEnvironment)
{
    ESP_LOGI(TAG, "Serial port closed");
    // Don't close the device here, it's managed externally
}

void SendComPortBlock_impl(ISP_ENVIRONMENT *IspEnvironment, const void *s, size_t n)
{
    if (s_cdc_dev == NULL || n == 0) {
        return;
    }
    
    // Ensure RTS stays high during bootloader operations
    ensure_rts_high();
    
    DumpString(4, s, n, "Sending ");
    
    // Use a reasonable timeout (500ms) instead of 0
    // The timeout of 0 might cause immediate timeout if the device buffer is full
    esp_err_t ret = cdc_acm_host_data_tx_blocking(s_cdc_dev, (const uint8_t *)s, n, 500);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send data: %s (len=%zu)", esp_err_to_name(ret), n);
        // Retry once after a short delay
        vTaskDelay(pdMS_TO_TICKS(50));
        ensure_rts_high();  // Ensure RTS is still high before retry
        ret = cdc_acm_host_data_tx_blocking(s_cdc_dev, (const uint8_t *)s, n, 500);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Retry also failed: %s", esp_err_to_name(ret));
        }
    }
    
    if (IspEnvironment->WriteDelay == 1) {
        vTaskDelay(pdMS_TO_TICKS(100)); // 100 ms delay
    }
}

// Wrapper for ESP32
void SendComPortBlock(ISP_ENVIRONMENT *IspEnvironment, const void *s, size_t n)
{
    SendComPortBlock_impl(IspEnvironment, s, n);
}

void SendComPort(ISP_ENVIRONMENT *IspEnvironment, const char *s)
{
    SendComPortBlock(IspEnvironment, s, strlen(s));
}

// Read data from the circular buffer (used by lpc21isp ReceiveComPort)
static void ReceiveComPortBlock(ISP_ENVIRONMENT *IspEnvironment,
                                void *answer, unsigned long max_size,
                                unsigned long *real_size)
{
    *real_size = 0;
    
    if (s_cdc_dev == NULL || max_size == 0 || answer == NULL || s_rx_buffer_mutex == NULL) {
        // Trigger timeout tick
        if (IspEnvironment->serial_timeout_count > 1) {
            IspEnvironment->serial_timeout_count--;
        } else {
            IspEnvironment->serial_timeout_count = 0;
        }
        return;
    }
    
    uint8_t *output = (uint8_t *)answer;
    size_t bytes_read = 0;
    
    // Try to read data from the circular buffer
    if (xSemaphoreTake(s_rx_buffer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        size_t available = (s_rx_write_pos >= s_rx_read_pos) ? 
                          (s_rx_write_pos - s_rx_read_pos) : 
                          (RX_BUFFER_SIZE - s_rx_read_pos + s_rx_write_pos);
        
        while (bytes_read < max_size && s_rx_read_pos != s_rx_write_pos) {
            output[bytes_read++] = s_rx_buffer[s_rx_read_pos];
            s_rx_read_pos = (s_rx_read_pos + 1) % RX_BUFFER_SIZE;
        }
        xSemaphoreGive(s_rx_buffer_mutex);
        
        if (bytes_read > 0) {
            // Debug: Log what we read (use ESP_LOG_BUFFER_HEX to save stack)
            size_t log_len = (bytes_read > 32) ? 32 : bytes_read;
            ESP_LOGD(TAG, "RX buffer: Read %zu bytes (avail was %zu)", bytes_read, available);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, output, log_len, ESP_LOG_DEBUG);
        } else if (available == 0) {
            ESP_LOGD(TAG, "RX buffer: Read 0 bytes, buffer empty (timeout_count=%lu)", 
                     IspEnvironment->serial_timeout_count);
        }
    }
    
    *real_size = bytes_read;
    
    // If no data was read, trigger timeout tick
    if (bytes_read == 0) {
        if (IspEnvironment->serial_timeout_count > 1) {
            IspEnvironment->serial_timeout_count--;
        } else {
            IspEnvironment->serial_timeout_count = 0;
        }
    }
}

void ReceiveComPort(ISP_ENVIRONMENT *IspEnvironment,
                    const char *Ans, unsigned long MaxSize,
                    unsigned long *RealSize, unsigned long WantedNr0x0A,
                    unsigned timeOutMilliseconds)
{
    unsigned long total_read = 0;
    unsigned long chunk_size = 0;
    char *answer_ptr = (char *)Ans;
    unsigned long timeout_ticks = timeOutMilliseconds / 100;
    
    ESP_LOGD(TAG, "ReceiveComPort: MaxSize=%lu, WantedNr0x0A=%lu, timeout=%ums", 
             MaxSize, WantedNr0x0A, timeOutMilliseconds);
    
    IspEnvironment->serial_timeout_count = timeout_ticks;
    
    while (total_read < MaxSize) {
        ReceiveComPortBlock(IspEnvironment, answer_ptr + total_read, 
                           MaxSize - total_read, &chunk_size);
        
        if (chunk_size > 0) {
            total_read += chunk_size;
            IspEnvironment->serial_timeout_count = timeout_ticks; // Reset timeout
            
            // Check for newline if required
            if (WantedNr0x0A > 0) {
                unsigned long newline_count = 0;
                for (unsigned long i = 0; i < total_read; i++) {
                    if (answer_ptr[i] == '\n') {
                        newline_count++;
                    }
                }
                if (newline_count >= WantedNr0x0A) {
                    ESP_LOGD(TAG, "ReceiveComPort: Got %lu newlines, stopping", newline_count);
                    break;
                }
            }
        } else {
            // No data received, check timeout
            if (IspEnvironment->serial_timeout_count == 0) {
                ESP_LOGD(TAG, "ReceiveComPort: Timeout, total_read=%lu", total_read);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
        }
    }
    
    *RealSize = total_read;
    if (total_read < MaxSize) {
        answer_ptr[total_read] = '\0'; // Null terminate
    }
    
    // Debug: Log what we received (use ESP_LOG_BUFFER_HEX to save stack)
    if (total_read > 0) {
        size_t log_len = (total_read > 64) ? 64 : total_read;
        ESP_LOGI(TAG, "ReceiveComPort: Received %lu bytes", total_read);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, answer_ptr, log_len, ESP_LOG_INFO);
    } else {
        ESP_LOGW(TAG, "ReceiveComPort: Received 0 bytes (timeout)");
    }
}

int ReceiveComPortBlockComplete(ISP_ENVIRONMENT *IspEnvironment, void *block, size_t size, unsigned timeout)
{
    unsigned long total_read = 0;
    unsigned long chunk_size = 0;
    uint8_t *block_ptr = (uint8_t *)block;
    unsigned long timeout_ticks = timeout / 100;
    
    IspEnvironment->serial_timeout_count = timeout_ticks;
    
    while (total_read < size) {
        ReceiveComPortBlock(IspEnvironment, block_ptr + total_read,
                           size - total_read, &chunk_size);
        
        if (chunk_size > 0) {
            total_read += chunk_size;
            IspEnvironment->serial_timeout_count = timeout_ticks; // Reset timeout
        } else {
            if (IspEnvironment->serial_timeout_count == 0) {
                return 0; // Timeout
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    return (total_read == size) ? 1 : 0;
}

void ClearSerialPortBuffers(ISP_ENVIRONMENT *IspEnvironment)
{
    if (s_rx_buffer_mutex == NULL) {
        return;
    }
    
    // Clear the circular buffer
    if (xSemaphoreTake(s_rx_buffer_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_rx_read_pos = s_rx_write_pos = 0;
        xSemaphoreGive(s_rx_buffer_mutex);
        ESP_LOGD(TAG, "Serial port RX buffer cleared");
    }
}

void ControlXonXoffSerialPort(ISP_ENVIRONMENT *IspEnvironment, unsigned char XonXoff)
{
    // CDC-ACM doesn't support XON/XOFF flow control in the same way
    // This is typically handled by the USB stack, so we can ignore it
    ESP_LOGD(TAG, "XON/XOFF control: %d (ignored for CDC-ACM)", XonXoff);
}

void ResetTarget(ISP_ENVIRONMENT *IspEnvironment, TARGET_MODE mode)
{
    if (s_cdc_dev == NULL) {
        ESP_LOGW(TAG, "ResetTarget called but CDC device is NULL");
        return;
    }
    
    // Only RTS is used (DTR is not connected)
    const bool dtr = false;  // DTR not connected, not used
    
    switch (mode) {
    case PROGRAM_MODE:
        // If ControlLines is disabled, we're handling reset manually
        // In this case, we've already synchronized, so just ensure RTS stays high
        if (IspEnvironment && IspEnvironment->ControlLines == 0) {
            // Already synchronized - just ensure RTS stays high without full reset sequence
            ESP_LOGD(TAG, "ResetTarget PROGRAM_MODE called but ControlLines disabled - ensuring RTS high");
            // Set flag and ensure RTS is high (even if flag wasn't set before)
            s_rts_keep_high = true;
            cdc_acm_host_set_control_line_state(s_cdc_dev, dtr, true);  // DTR=unused, RTS=high
            return;
        }
        
        // ControlLines is enabled - perform full reset sequence
        // This is needed to enter bootloader mode before manual reset
        // Set RTS high BEFORE user manually resets the device
        // RTS must be high when reset is pressed to enter bootloader mode
        ESP_LOGI(TAG, "Setting RTS high - must be high before reset");
        
        if (IspEnvironment) {
            ClearSerialPortBuffers(IspEnvironment);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Set flag to keep RTS high during bootloader operations
        s_rts_keep_high = true;
        
        // Pull RTS high - this enters bootloader mode
        // RTS must stay high for bootloader to remain active
        esp_err_t ret = cdc_acm_host_set_control_line_state(s_cdc_dev, dtr, true);  // DTR=unused, RTS=high
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set RTS high: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "RTS set to high - bootloader mode active (will be maintained)");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Verify RTS is still high (in case something overrode it)
        // RTS remains high - bootloader mode is maintained
        ret = cdc_acm_host_set_control_line_state(s_cdc_dev, dtr, true);  // Ensure RTS is still high
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to verify RTS high: %s", esp_err_to_name(ret));
        }
        break;
        
    case RUN_MODE:
        // Only perform RUN_MODE reset if ControlLines is enabled
        // Otherwise, skip it (we handle reset manually)
        if (IspEnvironment && IspEnvironment->ControlLines == 0) {
            ESP_LOGD(TAG, "ResetTarget RUN_MODE called but ControlLines disabled - skipping reset");
            return;
        }
        
        // Reset and start uploaded program
        ESP_LOGI(TAG, "Resetting to run mode");
        cdc_acm_host_set_control_line_state(s_cdc_dev, dtr, true);  // DTR=unused, RTS=high
        vTaskDelay(pdMS_TO_TICKS(100));
        
        ClearSerialPortBuffers(IspEnvironment);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Release RTS (set low) for normal operation
        s_rts_keep_high = false;  // Stop keeping RTS high
        cdc_acm_host_set_control_line_state(s_cdc_dev, dtr, false);  // DTR=unused, RTS=low
        vTaskDelay(pdMS_TO_TICKS(100));
        break;
    }
    
    ESP_LOGI(TAG, "Target reset complete: mode=%d", mode);
}

// Sleep function for ESP32
void Sleep(unsigned long MilliSeconds)
{
    vTaskDelay(pdMS_TO_TICKS(MilliSeconds));
}

