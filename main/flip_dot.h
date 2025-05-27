/**
 * @file flip_dot.h
 * @brief Flip dot display driver
 *
 * @author Mikael Bengtsson
 * @date 2025-05-18
 */

#ifndef FLIP_DOT_H
#define FLIP_DOT_H

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Public Definitions and Types
 ******************************************************************************/

// Display dimensions
#define DISPLAY_WIDTH 28
#define DISPLAY_HEIGHT 13

// Sweep modes
typedef enum {
    SWEEP_ROW,
    SWEEP_COL,
    SWEEP_DIAG,
    SWEEP_RANDOM
} sweep_mode_t;

// GPIO pin mapping
typedef struct {
    uint8_t pin;
    bool is_inverted;
} gpio_pin_t;

// 74HC4514 demultiplexer
typedef struct {
    gpio_pin_t pin_A0;
    gpio_pin_t pin_A1;
    gpio_pin_t pin_A2;
    gpio_pin_t pin_A3;  // A3 is optional, might be NULL for row demux
} demux_74HC4514_t;

// 74HC139 demultiplexer
typedef struct {
    gpio_pin_t pin_1A0;
    gpio_pin_t pin_1A1;
    gpio_pin_t pin_2A0;
    gpio_pin_t pin_2A1;
    gpio_pin_t pin_1E;
    gpio_pin_t pin_2E;
} demux_74HC139_t;

// FlipFlop display controller
typedef struct {
    demux_74HC139_t enable_demux;
    demux_74HC4514_t col_demux;
    demux_74HC4514_t row_demux;
    uint32_t flip_time_us;
    sweep_mode_t sweep_mode;
    uint8_t pixel_state[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} flip_dot_t;

/******************************************************************************
 * Public Constants
 ******************************************************************************/


/******************************************************************************
 * Public Function Declarations
 ******************************************************************************/

// Flip dot display functions
void flip_dot_init(flip_dot_t *display, uint32_t flip_time_us, sweep_mode_t sweep_mode);
void flip_dot_set_pixel(flip_dot_t *display, uint8_t row, uint8_t col, bool value);
void flip_dot_update_display(flip_dot_t *display, const uint8_t data[DISPLAY_HEIGHT][DISPLAY_WIDTH]);
void flip_dot_set_rows_cols(flip_dot_t *display, uint8_t row_start, uint8_t row_end, uint8_t col_start, uint8_t col_end, bool pixel_value);

// Debug functions
void flip_dot_debug_pixel_calc(uint8_t row, uint8_t col, bool value);

#endif /* FLIP_DOT_H */
