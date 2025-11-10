#ifndef RGB_LED_H
#define RGB_LED_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// LED states
typedef enum {
    LED_STATE_IDLE,
    LED_STATE_CONNECTING,
    LED_STATE_SYNCHRONIZING,
    LED_STATE_SYNCHRONIZED,
    LED_STATE_PROGRAMMING,
    LED_STATE_ERROR
} led_state_t;

/**
 * @brief Initialize RGB LED
 */
esp_err_t rgb_led_init(void);

/**
 * @brief Set LED state
 */
void rgb_led_set_state(led_state_t state);

/**
 * @brief Get current LED state
 */
led_state_t rgb_led_get_state(void);

#ifdef __cplusplus
}
#endif

#endif // RGB_LED_H

