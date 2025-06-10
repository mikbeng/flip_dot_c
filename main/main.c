#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwr_ctrl.h"
#include "esp_log.h"
#include "flip_dot.h"
#include "driver/gpio.h"

const static char *TAG = "MAIN";

static flip_dot_t flip_dot;

void app_main(void)
{
    init_power_control();
    int voltage_mv;

    get_battery_voltage(&voltage_mv);
    ESP_LOGI(TAG, "Battery voltage: %d mV", voltage_mv);

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

    ESP_LOGI(TAG, "Running bouncing ball demo...");
    flip_dot_demo_bouncing_ball(&flip_dot, 30);

    // Demo loop - cycle through different animations
    while (1) {
        // ESP_LOGI(TAG, "Running sine wave demo...");
        // flip_dot_demo_sine_wave(&flip_dot, 150);
        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        

        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running matrix rain demo...");
        // flip_dot_demo_matrix_rain(&flip_dot, 150);
        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running ripple effect demo...");
        // flip_dot_demo_ripple_effect(&flip_dot, 200);
        
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        // ESP_LOGI(TAG, "Running Game of Life demo...");
        // flip_dot_demo_game_of_life(&flip_dot, 500, 50);
        
        vTaskDelay(10000 / portTICK_PERIOD_MS);

    }
}
