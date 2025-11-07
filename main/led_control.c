/**
 * @file led_control.c
 * @brief WS2812B RGB LED Control Implementation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#include "led_control.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "led";

/* RMT configuration */
static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static int led_gpio_pin = -1;

/* LED state machine */
static led_state_t current_state = LED_STATE_IDLE;
static TaskHandle_t led_task_handle = NULL;

/* WS2812B timing configuration (in nanoseconds) */
#define WS2812_T0H_NS   (350)
#define WS2812_T0L_NS   (900)
#define WS2812_T1H_NS   (900)
#define WS2812_T1L_NS   (350)
#define WS2812_RESET_US (280)

/* LED brightness (0-255) */
#define LED_BRIGHTNESS  (50)  /* 20% brightness to avoid blinding */

/**
 * @brief WS2812B encoder structure
 */
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

/**
 * @brief Encode WS2812B data
 */
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                    const void *primary_data, size_t data_size,
                                    rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state) {
    case 0: /* send RGB data */
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; /* switch to next state when current encoding session finished */
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; /* yield if there's no free space to put other encoding artifacts */
        }
    /* fall-through */
    case 1: /* send reset code */
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                 sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; /* back to the initial encoding session */
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

/**
 * @brief Delete WS2812B encoder
 */
static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

/**
 * @brief Reset WS2812B encoder
 */
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

/**
 * @brief Create WS2812B encoder
 */
static esp_err_t rmt_new_led_strip_encoder(rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;

    led_encoder = calloc(1, sizeof(rmt_led_strip_encoder_t));
    if (!led_encoder) {
        return ESP_ERR_NO_MEM;
    }

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;

    /* WS2812B uses a special timing for 0 and 1 bits */
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = WS2812_T0H_NS / 12.5, /* 12.5ns resolution */
            .level1 = 0,
            .duration1 = WS2812_T0L_NS / 12.5,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = WS2812_T1H_NS / 12.5,
            .level1 = 0,
            .duration1 = WS2812_T1L_NS / 12.5,
        },
        .flags.msb_first = 1, /* WS2812B uses MSB first */
    };
    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    /* Create copy encoder for reset code */
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    /* Reset code: low level for 280us */
    /* At 80MHz resolution (12.5ns), 280us = 280000/12.5 = 22400 ticks */
    uint32_t reset_ticks = (WS2812_RESET_US * 1000) / 12.5; /* Convert us to ticks */
    
    /* RMT symbol can hold max 32767 (15 bits) per duration */
    if (reset_ticks > 0x7FFF) {
        /* Split into two durations if too long */
        led_encoder->reset_code = (rmt_symbol_word_t) {
            .level0 = 0,
            .duration0 = 0x7FFF,
            .level1 = 0,
            .duration1 = reset_ticks - 0x7FFF,
        };
    } else {
        led_encoder->reset_code = (rmt_symbol_word_t) {
            .level0 = 0,
            .duration0 = reset_ticks,
            .level1 = 0,
            .duration1 = 0,
        };
    }

    *ret_encoder = &led_encoder->base;
    return ESP_OK;

err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

/**
 * @brief Set LED color (RGB)
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 */
static esp_err_t led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (led_chan == NULL || led_encoder == NULL) {
        ESP_LOGE(TAG, "LED not initialized: chan=%p encoder=%p", led_chan, led_encoder);
        return ESP_ERR_INVALID_STATE;
    }

    /* Apply brightness scaling */
    r = (r * LED_BRIGHTNESS) / 255;
    g = (g * LED_BRIGHTNESS) / 255;
    b = (b * LED_BRIGHTNESS) / 255;

    /* WS2812B uses GRB format */
    uint8_t led_data[3] = {g, r, b};

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, /* no loop */
    };

    return rmt_transmit(led_chan, led_encoder, led_data, sizeof(led_data), &tx_config);
}

/**
 * @brief LED animation task
 */
static void led_animation_task(void *arg)
{
    ESP_LOGI(TAG, "LED animation task started");

    /* Wait for system to stabilize */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Verify initialization before starting */
    if (led_chan == NULL || led_encoder == NULL) {
        ESP_LOGE(TAG, "LED not initialized in task: chan=%p encoder=%p", led_chan, led_encoder);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "LED animation task running");

    while (1) {
        switch (current_state) {
            case LED_STATE_IDLE:
                /* Green slow blink (500ms ON / 1500ms OFF) */
                led_set_color(0, 255, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                led_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(1500));
                break;

            case LED_STATE_PREPARE:
                /* Cyan fast blink (300ms ON / 300ms OFF) */
                led_set_color(0, 255, 255);
                vTaskDelay(pdMS_TO_TICKS(300));
                led_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(300));
                break;

            case LED_STATE_COPY:
                /* Yellow blink (400ms ON / 400ms OFF) */
                led_set_color(255, 255, 0);
                vTaskDelay(pdMS_TO_TICKS(400));
                led_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(400));
                break;

            case LED_STATE_SYNC:
                /* Magenta blink (250ms ON / 250ms OFF) */
                led_set_color(255, 0, 255);
                vTaskDelay(pdMS_TO_TICKS(250));
                led_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                break;

            case LED_STATE_SUCCESS:
                /* Green solid for 2 seconds */
                led_set_color(0, 255, 0);
                vTaskDelay(pdMS_TO_TICKS(2000));
                /* Then back to IDLE */
                current_state = LED_STATE_IDLE;
                break;

            case LED_STATE_ERROR:
                /* Red fast blink (150ms ON / 150ms OFF) */
                led_set_color(255, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(150));
                led_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(150));
                break;

            default:
                /* Unknown state - turn off LED */
                led_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}

/**
 * @brief Initialize LED control
 */
esp_err_t led_control_init(int gpio_pin)
{
    ESP_LOGI(TAG, "Initializing LED on GPIO %d", gpio_pin);

    if (gpio_pin < 0) {
        ESP_LOGE(TAG, "Invalid GPIO pin: %d", gpio_pin);
        return ESP_ERR_INVALID_ARG;
    }

    led_gpio_pin = gpio_pin;

    /* Configure RMT TX channel */
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_pin,
        .mem_block_symbols = 64,
        .resolution_hz = 80 * 1000 * 1000, /* 80MHz, 12.5ns resolution */
        .trans_queue_depth = 4,
        .flags.invert_out = false,
        .flags.with_dma = false,
    };

    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Create LED strip encoder */
    ESP_LOGI(TAG, "Creating LED strip encoder...");
    ret = rmt_new_led_strip_encoder(&led_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED encoder: %s", esp_err_to_name(ret));
        rmt_del_channel(led_chan);
        led_chan = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "LED encoder created: %p", led_encoder);

    /* Enable RMT TX channel */
    ESP_LOGI(TAG, "Enabling RMT channel...");
    ret = rmt_enable(led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        rmt_del_encoder(led_encoder);
        rmt_del_channel(led_chan);
        led_encoder = NULL;
        led_chan = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "RMT channel enabled");

    /* Turn off LED initially */
    ESP_LOGI(TAG, "Setting initial LED state...");
    ret = led_set_color(0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set initial LED color: %s", esp_err_to_name(ret));
        /* Continue anyway - this might work later */
    }
    ESP_LOGI(TAG, "Initial LED state set");

    /* Wait a bit to ensure RMT is fully initialized */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Create LED animation task with larger stack */
    BaseType_t task_ret = xTaskCreate(
        led_animation_task,
        "led_anim",
        4096,  /* Increased stack size */
        NULL,
        5,
        &led_task_handle
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED animation task");
        rmt_disable(led_chan);
        rmt_del_encoder(led_encoder);
        rmt_del_channel(led_chan);
        led_encoder = NULL;
        led_chan = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LED control initialized successfully on GPIO %d", gpio_pin);
    return ESP_OK;
}

/**
 * @brief Set LED state
 */
esp_err_t led_control_set_state(led_state_t state)
{
    if (state > LED_STATE_ERROR) {
        ESP_LOGE(TAG, "Invalid LED state: %d", state);
        return ESP_ERR_INVALID_ARG;
    }

    const char *state_names[] = {
        "IDLE", "PREPARE", "COPY", "SYNC", "SUCCESS", "ERROR"
    };

    ESP_LOGI(TAG, "LED state changed: %s", state_names[state]);
    current_state = state;

    return ESP_OK;
}

/**
 * @brief Get current LED state
 */
led_state_t led_control_get_state(void)
{
    return current_state;
}

/**
 * @brief Deinitialize LED control
 */
esp_err_t led_control_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing LED");

    /* Delete animation task */
    if (led_task_handle != NULL) {
        vTaskDelete(led_task_handle);
        led_task_handle = NULL;
    }

    /* Turn off LED */
    if (led_chan != NULL && led_encoder != NULL) {
        led_set_color(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(100)); /* Wait for transmission to complete */
    }

    /* Disable and delete RMT channel */
    if (led_chan != NULL) {
        rmt_disable(led_chan);
        rmt_del_channel(led_chan);
        led_chan = NULL;
    }

    /* Delete encoder */
    if (led_encoder != NULL) {
        rmt_del_encoder(led_encoder);
        led_encoder = NULL;
    }

    ESP_LOGI(TAG, "LED control deinitialized");
    return ESP_OK;
}


