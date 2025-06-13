#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwr_ctrl.h"
#include "esp_log.h"
#include "flip_dot.h"
#include "snake.h"
#include "input.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_mac.h"

const static char *TAG = "MAIN";

static flip_dot_t flip_dot;

void app_main(void)
{
    // Print MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    ESP_LOGI(TAG, "Device MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    init_power_control();
    int voltage_mv;

    //Initialize flip dot
    flip_dot_init(&flip_dot, 2000, SWEEP_ROW);

    //Enable flip board
    ESP_LOGI(TAG, "Enabling flip board");
    disable_24V_supply();
    enable_flip_board();

    // Wait a bit for power to stabilize
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    //Clear display
    flip_dot_clear_display(&flip_dot);

    // Initialize input system with ESP-NOW
    input_system_config_t input_config = input_get_default_config();
    input_config.enabled_types = INPUT_TYPE_ESPNOW;  // Enable ESP-NOW input
    input_config.espnow_config = input_get_default_espnow_config();
    
    input_system_t input_sys;
    esp_err_t ret = input_system_init(&input_sys, &input_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize input system: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = input_system_start(&input_sys);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start input system: %s", esp_err_to_name(ret));
        input_system_deinit(&input_sys);
        return;
    }

    ESP_LOGI(TAG, "Starting interactive Snake game...");
    snake_game_run_interactive(&flip_dot);

    // Demo loop - cycle through different animations
    while (1) {
        ESP_LOGI(TAG, "Running bouncing ball demo...");
        flip_dot_demo_bouncing_ball(&flip_dot, 30);
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        
        ESP_LOGI(TAG, "Running Snake game demo...");
        snake_game_demo(&flip_dot, 30000);  // Run Snake demo for 30 seconds
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running sine wave demo...");
        // flip_dot_demo_sine_wave(&flip_dot, 150);
        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running matrix rain demo...");
        // flip_dot_demo_matrix_rain(&flip_dot, 150);
        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running ripple effect demo...");
        // flip_dot_demo_ripple_effect(&flip_dot, 200);
        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running Game of Life demo...");
        // flip_dot_demo_game_of_life(&flip_dot, 500, 50);

    }
}
