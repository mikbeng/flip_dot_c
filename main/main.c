#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwr_ctrl.h"
#include "esp_log.h"
#include "flip_dot.h"

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

    int i = 0;
    while (1) {
        //printf("[%d] Hello world!\n", i);
        flip_dot_set_pixel(&flip_dot, 0, 0, true);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
