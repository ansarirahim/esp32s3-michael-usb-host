/**
 * @file filesystem.h
 * @brief Internal FATFS Filesystem Interface
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#pragma once

#include <stddef.h>
#include "esp_err.h"

esp_err_t filesystem_init(void);
esp_err_t filesystem_deinit(void);
esp_err_t filesystem_read_label(char *label_buffer, size_t max_len);

