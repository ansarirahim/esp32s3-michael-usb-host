/*
 * ESP32 Wrapper for lpc21isp
 * 
 * This header provides a simplified API for using lpc21isp on ESP32
 */

#ifndef LPC21ISP_ESP32_H
#define LPC21ISP_ESP32_H

#include "usb/cdc_acm_host.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "lpc21isp.h"

/**
 * @brief Initialize lpc21isp for ESP32 with CDC-ACM device
 * 
 * @param cdc_dev CDC-ACM device handle
 * @param baud_rate Baud rate string (e.g., "38400")
 * @return ISP_ENVIRONMENT* Pointer to initialized ISP environment, or NULL on error
 */
ISP_ENVIRONMENT* lpc21isp_esp32_init(cdc_acm_dev_hdl_t cdc_dev, const char *baud_rate);

/**
 * @brief Program an LPC microcontroller
 * 
 * @param env ISP environment
 * @param firmware_data Pointer to firmware binary data
 * @param firmware_size Size of firmware data
 * @param start_address Start address for firmware
 * @return int 0 on success, error code on failure
 */
int lpc21isp_esp32_program(ISP_ENVIRONMENT *env, 
                           const uint8_t *firmware_data, 
                           size_t firmware_size,
                           unsigned long start_address);

/**
 * @brief Check if bootloader is synchronized (set by callback)
 * 
 * @return true if synchronized, false otherwise
 */
bool lpc21isp_esp32_is_synchronized(void);

/**
 * @brief Reset the synchronized flag
 */
void lpc21isp_esp32_reset_sync_flag(void);

/**
 * @brief Program device from hex file stored in FatFS
 * 
 * @param env ISP environment
 * @param hex_filename Filename of hex file in FatFS storage (e.g., "DM15_M80_34.hex")
 * @return int 0 on success, error code on failure
 */
int lpc21isp_esp32_program_from_hex_file(ISP_ENVIRONMENT *env, const char *hex_filename);

/**
 * @brief Send '?' sync characters to enter bootloader mode
 * 
 * This function sends '?' characters to the LPC microcontroller to enter bootloader mode.
 * The bootloader should echo back '?' and respond with "Synchronized\n" when ready.
 * 
 * @param env ISP environment
 * @param num_attempts Number of '?' characters to send (default: 100)
 * @param check_response If true, wait for and verify "Synchronized" response
 * @return int 0 on success, error code on failure
 */
int lpc21isp_esp32_sync_bootloader(ISP_ENVIRONMENT *env, int num_attempts, bool check_response);

/**
 * @brief Reset target microcontroller into program mode
 * 
 * This function toggles the DTR/RTS lines to reset the target into bootloader mode.
 * 
 * @param env ISP environment
 */
void lpc21isp_esp32_reset_to_bootloader(ISP_ENVIRONMENT *env);

/**
 * @brief Cleanup lpc21isp resources
 * 
 * @param env ISP environment
 */
void lpc21isp_esp32_cleanup(ISP_ENVIRONMENT *env);

#ifdef __cplusplus
}
#endif

#endif // LPC21ISP_ESP32_H

