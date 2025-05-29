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

    //Clear display
    flip_dot_clear_display(&flip_dot);

    bool pixel_value = true;
    while (1) {
        //Flip the first 5 pixels in the first row
        for (int i = 0; i < 5; i++) {
            flip_dot_set_pixel(&flip_dot, 0, i, pixel_value);
        }
        pixel_value = !pixel_value;
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
