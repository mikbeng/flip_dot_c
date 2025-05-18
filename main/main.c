#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwr_ctrl.h"
#include "esp_log.h"

const static char *TAG = "MAIN";

void app_main(void)
{
    init_power_control();
    int voltage_mv;

    //Enable flip board
    enable_flip_board();
    int i = 0;
    while (1) {
        //printf("[%d] Hello world!\n", i);
        get_battery_voltage(&voltage_mv);
        ESP_LOGI(TAG, "Battery voltage: %d mV", voltage_mv);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
