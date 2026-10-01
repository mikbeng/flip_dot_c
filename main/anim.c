/**
 * @file anim.c
 * @brief Flash-resident flipbook animation player
 */

#include "anim.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "anim";

#define ANIM_BYTES_PER_FRAME ((DISPLAY_WIDTH * DISPLAY_HEIGHT + 7) / 8)

static void unpack_frame(const uint8_t *packed, uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH])
{
    memset(buffer, 0, sizeof(uint8_t) * DISPLAY_HEIGHT * DISPLAY_WIDTH);

    for (uint8_t row = 0; row < DISPLAY_HEIGHT; row++) {
        for (uint8_t col = 0; col < DISPLAY_WIDTH; col++) {
            uint16_t idx = (uint16_t)row * DISPLAY_WIDTH + col;
            uint16_t byte_idx = idx / 8;
            uint8_t bit = (uint8_t)(7 - (idx % 8));
            if ((packed[byte_idx] >> bit) & 1) {
                buffer[row][col] = 1;
            }
        }
    }
}

static bool anim_frame_delay(uint32_t delay_ms, flip_dot_abort_cb_t should_abort)
{
    const uint32_t step_ms = 10;
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < delay_ms) {
        if (should_abort && should_abort()) {
            return true;
        }
        vTaskDelay(step_ms / portTICK_PERIOD_MS);
        elapsed_ms += step_ms;
    }
    return false;
}

bool anim_play(flip_dot_t *display, anim_id_t id, flip_dot_abort_cb_t should_abort)
{
    const anim_clip_t *clip = anim_clip_get(id);
    if (!clip || !clip->frames || clip->frame_count == 0) {
        ESP_LOGE(TAG, "Invalid animation id %d", (int)id);
        return false;
    }

    uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

    for (uint16_t frame = 0; frame < clip->frame_count; frame++) {
        if (should_abort && should_abort()) {
            return true;
        }

        const uint8_t *packed = clip->frames + (size_t)frame * ANIM_BYTES_PER_FRAME;
        unpack_frame(packed, buffer);
        flip_dot_update_display(display, buffer);

        if (anim_frame_delay(clip->delay_ms, should_abort)) {
            return true;
        }
    }

    return false;
}

bool anim_playlist_run(flip_dot_t *display, const anim_id_t *list, size_t count,
                       flip_dot_abort_cb_t should_abort)
{
    if (!list) {
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        if (anim_play(display, list[i], should_abort)) {
            return true;
        }
    }
    return false;
}
