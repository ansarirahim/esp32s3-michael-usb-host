/**
 * @file board_pins.c
 * @brief Hardware Pin Initialization and Board Detection
 *
 * @author Abdul Raheem Ansari <ansarirahim1@gmail.com>
 * @date November 2025
 * @version 1.0.0
 *
 * Implements MAC address-based board detection and GPIO auto-probing
 */

#include "board_pins.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "board_pins";

/* Board detection data */
static int detected_led_gpio = -1;
static const char *board_name = "Unknown";

/**
 * @brief Board detection table (MAC address -> GPIO mapping)
 */
typedef struct {
    uint8_t mac[6];
    int gpio;
    const char *name;
} board_info_t;

static const board_info_t board_table[] = {
    /* Espressif DevKitC-1-N8R8 (COM14) */
    {{0x98, 0xa3, 0x16, 0xf0, 0x6c, 0x64}, 38, "Espressif DevKitC-1-N8R8"},
    
    /* EWeAct ESP32-S3-DevKitC-1 (COM11) */
    {{0x48, 0xca, 0x43, 0xaf, 0x1e, 0x40}, 48, "EWeAct ESP32-S3-DevKitC-1"},
};

static const int board_table_size = sizeof(board_table) / sizeof(board_table[0]);

/**
 * @brief Detect board by MAC address
 */
static void detect_board_by_mac(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    ESP_LOGI(TAG, "Chip MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    for (int i = 0; i < board_table_size; i++) {
        if (memcmp(mac, board_table[i].mac, 6) == 0) {
            detected_led_gpio = board_table[i].gpio;
            board_name = board_table[i].name;
            ESP_LOGI(TAG, "✓ MAC matched: %s → GPIO %d", board_name, detected_led_gpio);
            return;
        }
    }
    
    ESP_LOGW(TAG, "⚠ MAC address not in detection table");
    ESP_LOGW(TAG, "Using default GPIO 38 (Espressif DevKitC-1-N8R8)");
    detected_led_gpio = 38;
    board_name = "Unknown (Default: GPIO38)";
}

/**
 * @brief Initialize board pins
 */
esp_err_t board_pins_init(void)
{
    ESP_LOGI(TAG, "Initializing board pins...");
    
    /* Detect board by MAC address */
    detect_board_by_mac();
    
    ESP_LOGI(TAG, "Board: %s", board_name);
    ESP_LOGI(TAG, "LED GPIO: %d", detected_led_gpio);
    
    return ESP_OK;
}

/**
 * @brief Get detected LED GPIO
 */
int board_pins_get_led_gpio(void)
{
    return detected_led_gpio;
}

/**
 * @brief Get board name
 */
const char* board_pins_get_board_name(void)
{
    return board_name;
}

