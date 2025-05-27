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
    flip_dot_init(&flip_dot, 3000, SWEEP_ROW);

    //Enable flip board
    ESP_LOGI(TAG, "Enabling flip board");
    enable_flip_board();

    // Wait a bit for power to stabilize
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Test systematic pixel mapping
    ESP_LOGI(TAG, "=== TESTING PIXEL MAPPING ===");
    
    // First, show the expected calculations for pixel (0,0)
    ESP_LOGI(TAG, "Expected calculations for pixel (0,0):");
    flip_dot_debug_pixel_calc(0, 0, true);
    
    // Test first few pixels in first row to see the pattern
    for (int col = 0; col < 7; col++) {
        ESP_LOGI(TAG, "\n--- Testing pixel (0,%d) ---", col);
        
        // Show expected calculation
        flip_dot_debug_pixel_calc(0, col, true);
        
        // Set pixel
        flip_dot_set_pixel(&flip_dot, 0, col, true);
        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Wait 2 seconds to observe
        
        // Clear pixel
        flip_dot_set_pixel(&flip_dot, 0, col, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second
    }
    
    ESP_LOGI(TAG, "=== COMPLETED FIRST ROW TEST ===");
    
    // Now test one pixel and leave it set
    ESP_LOGI(TAG, "Setting pixel (0,0) and leaving it on...");
    flip_dot_set_pixel(&flip_dot, 0, 0, true);
    
    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Pixel (0,0) should be ON. Check which physical pixel is lit.");
    }
}
