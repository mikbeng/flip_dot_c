/**
 * @file clock.c
 * @brief MM:SS clock application implementation
 */

#include "clock.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "clock";

#define FONT_WIDTH  5
#define FONT_HEIGHT 7
#define CHAR_GAP    1
#define COLON_WIDTH 2

/* 5x7 glyphs: each byte is one column, bits 0..6 are rows (top to bottom). */
static const uint8_t GLYPH_DIGITS[10][FONT_WIDTH] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
};

static const uint8_t GLYPH_COLON[COLON_WIDTH] = {0x00, 0x24};

static uint32_t s_start_ms;

static void clock_get_mm_ss(uint8_t *mm, uint8_t *ss)
{
    uint32_t elapsed_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - s_start_ms;
    uint32_t total_sec = elapsed_ms / 1000;
    *mm = (uint8_t)((total_sec / 60) % 60);
    *ss = (uint8_t)(total_sec % 60);
}

static void draw_glyph(uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH],
                       int x, int y,
                       const uint8_t *cols, int width)
{
    for (int col = 0; col < width; col++) {
        int px = x + col;
        if (px < 0 || px >= DISPLAY_WIDTH) {
            continue;
        }
        for (int row = 0; row < FONT_HEIGHT; row++) {
            int py = y + row;
            if (py < 0 || py >= DISPLAY_HEIGHT) {
                continue;
            }
            if ((cols[col] >> row) & 1) {
                buffer[py][px] = 1;
            }
        }
    }
}

static int clock_string_width(void)
{
    return (FONT_WIDTH + CHAR_GAP) * 2 + COLON_WIDTH + CHAR_GAP
         + (FONT_WIDTH + CHAR_GAP) * 2 - CHAR_GAP;
}

static void clock_render(uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH],
                         uint8_t mm, uint8_t ss)
{
    memset(buffer, 0, DISPLAY_HEIGHT * DISPLAY_WIDTH);

    const int total_w = clock_string_width();
    const int origin_x = (DISPLAY_WIDTH - total_w) / 2;
    const int origin_y = (DISPLAY_HEIGHT - FONT_HEIGHT) / 2;

    uint8_t digits[4] = {
        mm / 10, mm % 10,
        ss / 10, ss % 10,
    };

    int x = origin_x;
    for (int i = 0; i < 4; i++) {
        if (i == 2) {
            draw_glyph(buffer, x, origin_y, GLYPH_COLON, COLON_WIDTH);
            x += COLON_WIDTH + CHAR_GAP;
        }
        draw_glyph(buffer, x, origin_y, GLYPH_DIGITS[digits[i]], FONT_WIDTH);
        x += FONT_WIDTH;
        if (i < 3) {
            x += CHAR_GAP;
        }
    }
}

void clock_app_run(flip_dot_t *display, clock_app_abort_cb_t should_abort)
{
    uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
    int last_second = -1;

    s_start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "Elapsed MM:SS from mode entry");

    while (1) {
        if (should_abort && should_abort()) {
            break;
        }

        uint8_t mm;
        uint8_t ss;
        clock_get_mm_ss(&mm, &ss);

        if ((int)ss != last_second) {
            clock_render(buffer, mm, ss);
            flip_dot_update_display(display, buffer);
            last_second = (int)ss;
            ESP_LOGD(TAG, "%02u:%02u", mm, ss);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
