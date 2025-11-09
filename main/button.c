/**
 * @file button.c
 * @brief Button Handler with Triple-Press Detection Implementation
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 */

#include "button.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"

static const char *TAG = "button";

/* Button state */
static button_config_t button_config = {0};
static bool button_initialized = false;
static bool button_enabled = false;
static uint32_t press_count = 0;
static uint32_t total_press_count = 0;
static int64_t last_press_time = 0;
static int64_t first_press_time = 0;
static TimerHandle_t debounce_timer = NULL;
static TimerHandle_t triple_press_timer = NULL;
static volatile bool debounce_active = false;

/* Forward declarations */
static void IRAM_ATTR button_isr_handler(void* arg);
static void debounce_timer_callback(TimerHandle_t xTimer);
static void triple_press_timer_callback(TimerHandle_t xTimer);

/**
 * @brief GPIO ISR handler (called on button press)
 */
static void IRAM_ATTR button_isr_handler(void* arg)
{
    if (!button_enabled || debounce_active) {
        return;  /* Ignore if disabled or debouncing */
    }

    /* Start debounce timer */
    debounce_active = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerStartFromISR(debounce_timer, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Debounce timer callback (called after debounce period)
 */
static void debounce_timer_callback(TimerHandle_t xTimer)
{
    /* Check if button is still pressed (active low) */
    if (gpio_get_level(button_config.gpio_num) == 0) {
        int64_t current_time = esp_timer_get_time() / 1000;  /* Convert to milliseconds */

        /* Check minimum inter-press interval */
        if (press_count > 0 && (current_time - last_press_time) < button_config.min_inter_press_ms) {
            ESP_LOGW(TAG, "Button press too fast, ignoring (inter-press: %lld ms)", 
                     current_time - last_press_time);
            debounce_active = false;
            return;
        }

        /* Increment press count */
        press_count++;
        total_press_count++;
        last_press_time = current_time;

        ESP_LOGI(TAG, "Button press detected (%lu/3)", press_count);

        /* Start triple-press timer on first press */
        if (press_count == 1) {
            first_press_time = current_time;
            xTimerStart(triple_press_timer, 0);
        }

        /* Check for triple-press */
        if (press_count >= 3) {
            int64_t elapsed = current_time - first_press_time;
            ESP_LOGI(TAG, "Triple-press detected! (elapsed: %lld ms)", elapsed);

            /* Stop triple-press timer */
            xTimerStop(triple_press_timer, 0);

            /* Reset press count */
            press_count = 0;

            /* Call callback */
            if (button_config.callback != NULL) {
                button_config.callback(button_config.user_data);
            }
        }
    } else {
        ESP_LOGD(TAG, "Button released before debounce complete, ignoring");
    }

    debounce_active = false;
}

/**
 * @brief Triple-press timer callback (called when triple-press window expires)
 */
static void triple_press_timer_callback(TimerHandle_t xTimer)
{
    if (press_count > 0 && press_count < 3) {
        ESP_LOGI(TAG, "Triple-press timeout, resetting (press_count: %lu)", press_count);
        press_count = 0;
    }
}

/**
 * @brief Initialize button handler
 */
esp_err_t button_init(const button_config_t* config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config (NULL)");
        return ESP_ERR_INVALID_ARG;
    }

    if (button_initialized) {
        ESP_LOGW(TAG, "Button already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing button handler on GPIO %d", config->gpio_num);

    /* Copy configuration */
    button_config = *config;

    /* Set default values if not specified */
    if (button_config.debounce_ms == 0) {
        button_config.debounce_ms = 50;  /* Default: 50ms */
    }
    if (button_config.triple_press_window_ms == 0) {
        button_config.triple_press_window_ms = 2000;  /* Default: 2000ms */
    }
    if (button_config.min_inter_press_ms == 0) {
        button_config.min_inter_press_ms = 150;  /* Default: 150ms */
    }

    ESP_LOGI(TAG, "Button config: debounce=%lu ms, window=%lu ms, min_inter_press=%lu ms",
             button_config.debounce_ms, button_config.triple_press_window_ms, 
             button_config.min_inter_press_ms);

    /* Configure GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  /* Enable internal pull-up */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  /* Trigger on falling edge (button press) */
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Create debounce timer */
    debounce_timer = xTimerCreate(
        "button_debounce",
        pdMS_TO_TICKS(button_config.debounce_ms),
        pdFALSE,  /* One-shot timer */
        NULL,
        debounce_timer_callback
    );

    if (debounce_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create debounce timer");
        return ESP_ERR_NO_MEM;
    }

    /* Create triple-press timer */
    triple_press_timer = xTimerCreate(
        "button_triple_press",
        pdMS_TO_TICKS(button_config.triple_press_window_ms),
        pdFALSE,  /* One-shot timer */
        NULL,
        triple_press_timer_callback
    );

    if (triple_press_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create triple-press timer");
        xTimerDelete(debounce_timer, 0);
        debounce_timer = NULL;
        return ESP_ERR_NO_MEM;
    }

    /* Install GPIO ISR service */
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE means ISR service already installed (OK) */
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(ret));
        xTimerDelete(debounce_timer, 0);
        xTimerDelete(triple_press_timer, 0);
        debounce_timer = NULL;
        triple_press_timer = NULL;
        return ret;
    }

    /* Add ISR handler */
    ret = gpio_isr_handler_add(config->gpio_num, button_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add GPIO ISR handler: %s", esp_err_to_name(ret));
        xTimerDelete(debounce_timer, 0);
        xTimerDelete(triple_press_timer, 0);
        debounce_timer = NULL;
        triple_press_timer = NULL;
        return ret;
    }

    button_initialized = true;
    button_enabled = true;  /* Enable by default */
    press_count = 0;
    total_press_count = 0;
    last_press_time = 0;
    first_press_time = 0;
    debounce_active = false;

    ESP_LOGI(TAG, "Button handler initialized successfully");
    return ESP_OK;
}

/**
 * @brief Deinitialize button handler
 */
esp_err_t button_deinit(void)
{
    if (!button_initialized) {
        ESP_LOGW(TAG, "Button not initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing button handler...");

    /* Disable button */
    button_enabled = false;

    /* Remove ISR handler */
    gpio_isr_handler_remove(button_config.gpio_num);

    /* Delete timers */
    if (debounce_timer != NULL) {
        xTimerDelete(debounce_timer, portMAX_DELAY);
        debounce_timer = NULL;
    }

    if (triple_press_timer != NULL) {
        xTimerDelete(triple_press_timer, portMAX_DELAY);
        triple_press_timer = NULL;
    }

    button_initialized = false;
    press_count = 0;

    ESP_LOGI(TAG, "Button handler deinitialized");
    return ESP_OK;
}

/**
 * @brief Enable button handler
 */
esp_err_t button_enable(void)
{
    if (!button_initialized) {
        ESP_LOGE(TAG, "Button not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    button_enabled = true;
    ESP_LOGI(TAG, "Button handler enabled");
    return ESP_OK;
}

/**
 * @brief Disable button handler
 */
esp_err_t button_disable(void)
{
    if (!button_initialized) {
        ESP_LOGE(TAG, "Button not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    button_enabled = false;
    press_count = 0;  /* Reset press count when disabled */
    ESP_LOGI(TAG, "Button handler disabled");
    return ESP_OK;
}

/**
 * @brief Check if button handler is enabled
 */
bool button_is_enabled(void)
{
    return button_enabled;
}

/**
 * @brief Get button press count
 */
uint32_t button_get_press_count(void)
{
    return total_press_count;
}

/**
 * @brief Reset button press count
 */
void button_reset_press_count(void)
{
    total_press_count = 0;
    ESP_LOGI(TAG, "Button press count reset");
}

