/*
 * ESP32 Serial Adapter for lpc21isp
 */

#ifndef ESP32_SERIAL_ADAPTER_H
#define ESP32_SERIAL_ADAPTER_H

#include "usb/cdc_acm_host.h"

// Include lpc21isp.h to get the ISP_ENVIRONMENT typedef
#include "lpc21isp.h"

#ifdef __cplusplus
extern "C" {
#endif

void esp32_serial_adapter_set_device(cdc_acm_dev_hdl_t cdc_dev);
void esp32_serial_adapter_get_device(cdc_acm_dev_hdl_t *cdc_dev);

// Function to add received data to the serial buffer (called from USB callback)
void esp32_serial_adapter_add_rx_data(const uint8_t *data, size_t data_len);

// Serial port functions for ESP32
void OpenSerialPort(ISP_ENVIRONMENT *IspEnvironment);
void CloseSerialPort(ISP_ENVIRONMENT *IspEnvironment);

#ifdef __cplusplus
}
#endif

#endif // ESP32_SERIAL_ADAPTER_H

