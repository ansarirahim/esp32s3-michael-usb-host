/*
 * RGB LED Control for ESP32-S3 DevKitC-1
 * 
 * Controls the onboard WS2812B RGB LED on pin 38 to display different states
 */

#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "led_strip_encoder.h"
#include "rgb_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "rgb_led";

// RGB LED pin (onboard WS2812B on ESP32-S3 DevKitC-1)
// Note: Some boards use GPIO 38, others use GPIO 48 - try both if LED doesn't work
#define RGB_LED_PIN GPIO_NUM_48  // Changed from GPIO_NUM_38 to match working configuration
#define RMT_RES_HZ  (10 * 1000 * 1000)  // 10 MHz resolution

// Brightness multiplier (0-255, where 255 = full brightness)
// Lower values reduce brightness: 32 = ~12.5%, 16 = ~6.3%, 8 = ~3.1%
#define LED_BRIGHTNESS 32

// Base color definitions (RGB values 0-255 at full brightness)
static const uint8_t COLOR_OFF_BASE[3]     = {0, 0, 0};
static const uint8_t COLOR_RED_BASE[3]     = {255, 0, 0};
static const uint8_t COLOR_GREEN_BASE[3]   = {0, 255, 0};
static const uint8_t COLOR_BLUE_BASE[3]    = {0, 0, 255};
static const uint8_t COLOR_YELLOW_BASE[3]  = {255, 255, 0};
static const uint8_t COLOR_ORANGE_BASE[3]  = {255, 165, 0};
static const uint8_t COLOR_CYAN_BASE[3]    = {0, 255, 255};
static const uint8_t COLOR_MAGENTA_BASE[3] = {255, 0, 255};
static const uint8_t COLOR_WHITE_BASE[3]   = {255, 255, 255};

// Apply brightness scaling to base colors
#define SCALE_COLOR(c) ((uint8_t)((c) * LED_BRIGHTNESS / 255))
static const uint8_t COLOR_OFF[3]     = {0, 0, 0};
static const uint8_t COLOR_RED[3]     = {SCALE_COLOR(255), 0, 0};
static const uint8_t COLOR_GREEN[3]   = {0, SCALE_COLOR(255), 0};
static const uint8_t COLOR_BLUE[3]    = {0, 0, SCALE_COLOR(255)};
static const uint8_t COLOR_YELLOW[3]  = {SCALE_COLOR(255), SCALE_COLOR(255), 0};
static const uint8_t COLOR_ORANGE[3]  = {SCALE_COLOR(255), SCALE_COLOR(165), 0};
static const uint8_t COLOR_CYAN[3]    = {0, SCALE_COLOR(255), SCALE_COLOR(255)};
static const uint8_t COLOR_MAGENTA[3] = {SCALE_COLOR(255), 0, SCALE_COLOR(255)};
static const uint8_t COLOR_WHITE[3]   = {SCALE_COLOR(255), SCALE_COLOR(255), SCALE_COLOR(255)};

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static led_state_t current_state = LED_STATE_IDLE;
static TaskHandle_t blink_task_handle = NULL;
static volatile bool blink_task_running = false;

// Forward declaration for static function
static void rgb_led_set_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Blinking task for SYNCHRONIZING and PROGRAMMING states
 */
static void rgb_led_blink_task(void *arg) {
    bool led_on = false;
    while (blink_task_running) {
        if (current_state == LED_STATE_SYNCHRONIZING) {
            if (led_on) {
                rgb_led_set_color(COLOR_YELLOW[0], COLOR_YELLOW[1], COLOR_YELLOW[2]);
            } else {
                rgb_led_set_color(COLOR_OFF[0], COLOR_OFF[1], COLOR_OFF[2]);
            }
            led_on = !led_on;
            vTaskDelay(pdMS_TO_TICKS(500)); // Blink every 500ms for synchronizing
        } else if (current_state == LED_STATE_PROGRAMMING) {
            if (led_on) {
                rgb_led_set_color(COLOR_BLUE[0], COLOR_BLUE[1], COLOR_BLUE[2]);
            } else {
                rgb_led_set_color(COLOR_OFF[0], COLOR_OFF[1], COLOR_OFF[2]);
            }
            led_on = !led_on;
            vTaskDelay(pdMS_TO_TICKS(100)); // Rapid blink every 100ms for programming
        } else {
            // State changed away from blinking states, exit
            break;
        }
    }
    blink_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief Set RGB LED color (static helper function)
 */
static void rgb_led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (led_chan == NULL || led_encoder == NULL) {
        ESP_LOGW(TAG, "LED channel or encoder not initialized (chan=%p, enc=%p)", 
                 (void *)led_chan, (void *)led_encoder);
        return;
    }

    // WS2812B uses GRB order (not RGB)
    uint8_t grb[3] = {g, r, b};
    
    rmt_transmit_config_t tx_config = {};
    tx_config.loop_count = 0;  // no loop
    
    esp_err_t ret = rmt_transmit(led_chan, led_encoder, grb, sizeof(grb), &tx_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit RGB data: %s", esp_err_to_name(ret));
        return;
    }
    
    // Wait for transmission to complete
    ret = rmt_tx_wait_all_done(led_chan, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for LED transmission: %s", esp_err_to_name(ret));
    }
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize RGB LED
 */
esp_err_t rgb_led_init(void) {
    ESP_LOGI(TAG, "Initializing RGB LED on pin %d", RGB_LED_PIN);

    // RMT transmitter configuration
    rmt_tx_channel_config_t tx_chan_config = {};
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.gpio_num = RGB_LED_PIN;
    tx_chan_config.mem_block_symbols = 64;
    tx_chan_config.resolution_hz = RMT_RES_HZ;
    tx_chan_config.trans_queue_depth = 4;
    
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create LED strip encoder (for WS2812B)
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_RES_HZ,
    };
    
    ret = rmt_new_led_strip_encoder(&encoder_config, &led_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip encoder: %s", esp_err_to_name(ret));
        rmt_del_channel(led_chan);
        led_chan = NULL;
        return ret;
    }

    // Enable RMT channel
    ret = rmt_enable(led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        rmt_del_encoder(led_encoder);
        rmt_del_channel(led_chan);
        led_encoder = NULL;
        led_chan = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "RGB LED initialized successfully");
    return ESP_OK;
}

/**
 * @brief Set LED state (with predefined colors)
 */
void rgb_led_set_state(led_state_t state) {
    // Safety check: don't crash if LED isn't initialized
    if (led_chan == NULL || led_encoder == NULL) {
        ESP_LOGW(TAG, "LED not initialized, skipping state change to %d", state);
        return;
    }
    
    // Stop blinking task if it's running and we're changing away from blinking states
    if (blink_task_running && 
        (current_state == LED_STATE_SYNCHRONIZING || current_state == LED_STATE_PROGRAMMING) &&
        state != LED_STATE_SYNCHRONIZING && state != LED_STATE_PROGRAMMING) {
        blink_task_running = false;
        // Wait a bit for task to finish
        vTaskDelay(pdMS_TO_TICKS(150));
        if (blink_task_handle != NULL) {
            vTaskDelete(blink_task_handle);
            blink_task_handle = NULL;
        }
    }
    
    current_state = state;
    
    uint8_t color[3];
    
    switch (state) {
        case LED_STATE_IDLE:
            memcpy(color, COLOR_OFF, sizeof(COLOR_OFF));
            rgb_led_set_color(color[0], color[1], color[2]);
            break;
        case LED_STATE_CONNECTING:
            // Magenta - solid color while waiting for device
            memcpy(color, COLOR_MAGENTA, sizeof(COLOR_MAGENTA));
            rgb_led_set_color(color[0], color[1], color[2]);
            break;
        case LED_STATE_SYNCHRONIZING:
            // Start blinking task for SYNCHRONIZING state
            if (!blink_task_running) {
                blink_task_running = true;
                xTaskCreate(rgb_led_blink_task, "led_blink", 2048, NULL, 5, &blink_task_handle);
            }
            break;
        case LED_STATE_PROGRAMMING:
            // Start rapid blinking task for PROGRAMMING state
            if (!blink_task_running) {
                blink_task_running = true;
                xTaskCreate(rgb_led_blink_task, "led_blink", 2048, NULL, 5, &blink_task_handle);
            }
            break;
        case LED_STATE_SYNCHRONIZED:
            // Green - distinct
            memcpy(color, COLOR_GREEN, sizeof(COLOR_GREEN));
            rgb_led_set_color(color[0], color[1], color[2]);
            break;
        case LED_STATE_ERROR:
            // Red - distinct
            memcpy(color, COLOR_RED, sizeof(COLOR_RED));
            rgb_led_set_color(color[0], color[1], color[2]);
            break;
        default:
            memcpy(color, COLOR_OFF, sizeof(COLOR_OFF));
            rgb_led_set_color(color[0], color[1], color[2]);
            break;
    }
}

/**
 * @brief Get current LED state
 */
led_state_t rgb_led_get_state(void) {
    return current_state;
}

#ifdef __cplusplus
}
#endif
