/**
 * @file usb_mode_nvs.h
 * @brief USB Mode NVS Storage Header
 * 
 * Manages USB mode persistence in NVS (Non-Volatile Storage).
 */

#ifndef USB_MODE_NVS_H
#define USB_MODE_NVS_H

#include "esp_err.h"
#include "usb_mode_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NVS
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t usb_mode_nvs_init(void);

/**
 * @brief Write USB mode to NVS
 * 
 * @param mode Mode to write
 * @return esp_err_t ESP_OK on success
 */
esp_err_t usb_mode_nvs_write(usb_mode_t mode);

/**
 * @brief Read USB mode from NVS
 * 
 * @param mode Pointer to store the read mode
 * @return esp_err_t ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if not found
 */
esp_err_t usb_mode_nvs_read(usb_mode_t *mode);

/**
 * @brief Toggle USB mode in NVS and return new mode
 * 
 * @param new_mode Pointer to store the new mode
 * @return esp_err_t ESP_OK on success
 */
esp_err_t usb_mode_nvs_toggle(usb_mode_t *new_mode);

#ifdef __cplusplus
}
#endif

#endif /* USB_MODE_NVS_H */

