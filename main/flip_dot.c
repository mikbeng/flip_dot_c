/**
 * @file flip_dot.c
 * @brief Flip dot display driver implementation
 *
 * @author Mikael Bengtsson
 * @date 2025-05-18
 */

#include "flip_dot.h"
#include <string.h>  
#include "driver/gpio.h" 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"
#include "esp_log.h"

/******************************************************************************
 * Private Definitions and Types
 ******************************************************************************/

// Pin definitions - replace with your actual pin numbers
#define PIN_ROW_A0 10
#define PIN_ROW_A1 11
#define PIN_ROW_A2 12
#define PIN_COL_A0 6
#define PIN_COL_A1 7
#define PIN_COL_A2 8
#define PIN_COL_A3 9
#define PIN_ENABLE_1A0 0
#define PIN_ENABLE_1A1 1
#define PIN_ENABLE_2A0 2
#define PIN_ENABLE_2A1 3
#define PIN_ENABLE_1E 4
#define PIN_ENABLE_2E 5

const char *TAG = "flip_dot";

/******************************************************************************
 * Private Function Declarations
 ******************************************************************************/
// GPIO functions
void gpio_write(uint8_t pin, bool value, bool is_inverted);

// Demux functions
void demux_74HC4514_init(demux_74HC4514_t *demux, 
                          gpio_pin_t pin_A0, 
                          gpio_pin_t pin_A1, 
                          gpio_pin_t pin_A2, 
                          gpio_pin_t pin_A3);
                          
void demux_74HC4514_set_output(demux_74HC4514_t *demux, uint8_t output_pos);

void demux_74HC139_init(demux_74HC139_t *demux,
                         gpio_pin_t pin_1A0,
                         gpio_pin_t pin_1A1,
                         gpio_pin_t pin_2A0,
                         gpio_pin_t pin_2A1,
                         gpio_pin_t pin_1E,
                         gpio_pin_t pin_2E);
                         
void demux_74HC139_set_row_output(demux_74HC139_t *demux, 
                                  uint8_t row_grp, 
                                  uint8_t output_pos, 
                                  demux_74HC4514_t *row_demux);
                                  
void demux_74HC139_set_col_output(demux_74HC139_t *demux, 
                                  uint8_t col_grp, 
                                  uint8_t output_pos, 
                                  demux_74HC4514_t *col_demux);
                                  
void demux_74HC139_enable_output(demux_74HC139_t *demux, uint8_t channel);
void demux_74HC139_disable_output(demux_74HC139_t *demux, uint8_t channel);

/******************************************************************************
 * Private Function Implementations
 ******************************************************************************/
// Delay function (microseconds)
static void delay_us(uint32_t us) {
    // Replace with your platform's microsecond delay
    vTaskDelay(us / 1000 / portTICK_PERIOD_MS);
}

// Delay function (milliseconds)
static void delay_ms(uint32_t ms) {
    // Replace with your platform's millisecond delay
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

// Converts a decimal number to binary array
static void decimal_to_bin(uint8_t number, uint8_t bits, uint8_t *binary_arr) {
    for (int i = bits-1; i >= 0; i--) {
        uint8_t mask = 1 << i;
        binary_arr[bits-1-i] = (mask & number) ? 1 : 0;
    }
}

/******************************************************************************
 * Public Function Implementations
 ******************************************************************************/

/******************************************************************************
 * GPIO Functions
 ******************************************************************************/

void gpio_write(uint8_t pin, bool value, bool is_inverted) {
    // Write value to GPIO pin, considering inversion
    gpio_set_level(pin, is_inverted ? !value : value);
}

/******************************************************************************
 * Demux Functions
 ******************************************************************************/

void demux_74HC4514_init(demux_74HC4514_t *demux, 
                          gpio_pin_t pin_A0, 
                          gpio_pin_t pin_A1, 
                          gpio_pin_t pin_A2, 
                          gpio_pin_t pin_A3) {
    demux->pin_A0 = pin_A0;
    demux->pin_A1 = pin_A1;
    demux->pin_A2 = pin_A2;
    demux->pin_A3 = pin_A3;

    //Initiaze GPIOs
    gpio_config_t io_config = {   
        .pin_bit_mask = (1ULL << pin_A0.pin) | (1ULL << pin_A1.pin) | (1ULL << pin_A2.pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (pin_A3.pin != 0) { // 0 indicates unused pin
        io_config.pin_bit_mask |= (1ULL << pin_A3.pin);
    }

    gpio_config(&io_config);

}

void demux_74HC4514_set_output(demux_74HC4514_t *demux, uint8_t output_pos) {
    uint8_t binary[4] = {0};
    
    // Convert output position to binary
    decimal_to_bin(output_pos, 4, binary);
    
    // Set output pins
    gpio_write(demux->pin_A0.pin, binary[3], demux->pin_A0.is_inverted);
    gpio_write(demux->pin_A1.pin, binary[2], demux->pin_A1.is_inverted);
    gpio_write(demux->pin_A2.pin, binary[1], demux->pin_A2.is_inverted);
    
    if (demux->pin_A3.pin != 0) {
        gpio_write(demux->pin_A3.pin, !binary[0], demux->pin_A3.is_inverted); // A3 is inverted in the original code
    }
}

void demux_74HC139_init(demux_74HC139_t *demux,
                         gpio_pin_t pin_1A0,
                         gpio_pin_t pin_1A1,
                         gpio_pin_t pin_2A0,
                         gpio_pin_t pin_2A1,
                         gpio_pin_t pin_1E,
                         gpio_pin_t pin_2E) {
    demux->pin_1A0 = pin_1A0;
    demux->pin_1A1 = pin_1A1;
    demux->pin_2A0 = pin_2A0;
    demux->pin_2A1 = pin_2A1;
    demux->pin_1E = pin_1E;
    demux->pin_2E = pin_2E;
    
    //Initiaze GPIOs
    gpio_config_t io_config = {   
        .pin_bit_mask = (1ULL << pin_1A0.pin) | (1ULL << pin_1A1.pin) | (1ULL << pin_2A0.pin) | (1ULL << pin_2A1.pin) | (1ULL << pin_1E.pin) | (1ULL << pin_2E.pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);
}

void demux_74HC139_set_row_output(demux_74HC139_t *demux, 
                                  uint8_t row_grp, 
                                  uint8_t output_pos, 
                                  demux_74HC4514_t *row_demux) {
    uint8_t binary[2] = {0};
    
    // Convert row group to binary
    decimal_to_bin(row_grp, 2, binary);
    
    // Set row group pins
    gpio_write(demux->pin_1A0.pin, binary[1], demux->pin_1A0.is_inverted);
    gpio_write(demux->pin_1A1.pin, binary[0], demux->pin_1A1.is_inverted);
    
    // Set row output position
    demux_74HC4514_set_output(row_demux, output_pos);
}

void demux_74HC139_set_col_output(demux_74HC139_t *demux, 
                                  uint8_t col_grp, 
                                  uint8_t output_pos, 
                                  demux_74HC4514_t *col_demux) {
    uint8_t binary[2] = {0};
    
    // Convert column group to binary
    decimal_to_bin(col_grp, 2, binary);
    
    // Set column group pins
    gpio_write(demux->pin_2A0.pin, binary[1], demux->pin_2A0.is_inverted);
    gpio_write(demux->pin_2A1.pin, binary[0], demux->pin_2A1.is_inverted);
    
    // Set column output position
    demux_74HC4514_set_output(col_demux, output_pos);
}

void demux_74HC139_enable_output(demux_74HC139_t *demux, uint8_t channel) {
    if (channel == 1) {
        gpio_write(demux->pin_1E.pin, true, demux->pin_1E.is_inverted);
    } else if (channel == 2) {
        gpio_write(demux->pin_2E.pin, true, demux->pin_2E.is_inverted);
    }
}

void demux_74HC139_disable_output(demux_74HC139_t *demux, uint8_t channel) {
    if (channel == 1) {
        gpio_write(demux->pin_1E.pin, false, demux->pin_1E.is_inverted);
    } else if (channel == 2) {
        gpio_write(demux->pin_2E.pin, false, demux->pin_2E.is_inverted);
    }
}

/******************************************************************************
 * Flip Dot Functions
 ******************************************************************************/

void flip_dot_init(flip_dot_t *display, uint32_t flip_time_us, sweep_mode_t sweep_mode) {
    
    ESP_LOGI(TAG, "Initializing flip dot");
    // Setup pin configurations
    gpio_pin_t row_A0 = {PIN_ROW_A0, false};
    gpio_pin_t row_A1 = {PIN_ROW_A1, false};
    gpio_pin_t row_A2 = {PIN_ROW_A2, false};
    gpio_pin_t row_A3 = {0, false};  // Not used for rows
    
    gpio_pin_t col_A0 = {PIN_COL_A0, false};
    gpio_pin_t col_A1 = {PIN_COL_A1, false};
    gpio_pin_t col_A2 = {PIN_COL_A2, false};
    gpio_pin_t col_A3 = {PIN_COL_A3, true};
    
    gpio_pin_t enable_1A0 = {PIN_ENABLE_1A0, false};
    gpio_pin_t enable_1A1 = {PIN_ENABLE_1A1, false};
    gpio_pin_t enable_2A0 = {PIN_ENABLE_2A0, false};
    gpio_pin_t enable_2A1 = {PIN_ENABLE_2A1, false};
    gpio_pin_t enable_1E = {PIN_ENABLE_1E, false};
    gpio_pin_t enable_2E = {PIN_ENABLE_2E, true};
    
    // Initialize demuxes
    demux_74HC4514_init(&display->row_demux, row_A0, row_A1, row_A2, row_A3);
    demux_74HC4514_init(&display->col_demux, col_A0, col_A1, col_A2, col_A3);
    demux_74HC139_init(&display->enable_demux, enable_1A0, enable_1A1, enable_2A0, enable_2A1, enable_1E, enable_2E);
    
    // Set parameters
    display->flip_time_us = flip_time_us;
    display->sweep_mode = sweep_mode;
    
    // Initialize pixel state to all zeros
    memset(display->pixel_state, 0, sizeof(display->pixel_state));
    
    // Enable row output
    demux_74HC139_enable_output(&display->enable_demux, 1);
    gpio_write(enable_2E.pin, false, enable_2E.is_inverted);
    delay_ms(200);
    
    ESP_LOGI(TAG, "Clearing all pixels");
    // Clear all pixels
    for (uint8_t r = 0; r < DISPLAY_HEIGHT; r++) {
        for (uint8_t c = 0; c < DISPLAY_WIDTH; c++) {
            flip_dot_set_pixel(display, r, c, 0);
        }
    }
    
    delay_ms(200);
}

void flip_dot_set_pixel(flip_dot_t *display, uint8_t row, uint8_t col, bool value) {
    uint8_t row_grp = row / 7;
    uint8_t row_grp_pixel = row % 7;
    
    uint8_t col_grp = col / 7;
    uint8_t col_grp_pixel = col % 7;
    
    // Set row output
    uint8_t row_output_pos = row_grp_pixel + 1 + (value * 8);
    ESP_LOGI(TAG, "Setting row output to %d", row_output_pos);
    demux_74HC139_set_row_output(&display->enable_demux, row_grp, row_output_pos, &display->row_demux);
    
    // Set column output
    uint8_t col_output_pos = col_grp_pixel + 1 + (value * 8);
    ESP_LOGI(TAG, "Setting column output to %d", col_output_pos);
    demux_74HC139_set_col_output(&display->enable_demux, col_grp, col_output_pos, &display->col_demux);
    
    // Send column enable pulse
    gpio_write(display->enable_demux.pin_2E.pin, true, display->enable_demux.pin_2E.is_inverted);
    delay_us(display->flip_time_us);
    gpio_write(display->enable_demux.pin_2E.pin, false, display->enable_demux.pin_2E.is_inverted);
    
    // Update pixel state in memory
    display->pixel_state[row][col] = value;
}

void flip_dot_update_display(flip_dot_t *display, const uint8_t data[DISPLAY_HEIGHT][DISPLAY_WIDTH]) {
    // Find which pixels need to change
    uint8_t flip_list[DISPLAY_HEIGHT * DISPLAY_WIDTH][2];
    uint16_t flip_count = 0;
    
    for (uint8_t r = 0; r < DISPLAY_HEIGHT; r++) {
        for (uint8_t c = 0; c < DISPLAY_WIDTH; c++) {
            if (data[r][c] != display->pixel_state[r][c]) {
                flip_list[flip_count][0] = r;
                flip_list[flip_count][1] = c;
                flip_count++;
            }
        }
    }
    
    // Sort flip list according to sweep mode
    switch (display->sweep_mode) {
        case SWEEP_ROW:
            // Already in row-by-row order
            break;
            
        case SWEEP_COL:
            // Sort by column
            for (uint16_t i = 0; i < flip_count; i++) {
                for (uint16_t j = i + 1; j < flip_count; j++) {
                    if (flip_list[i][1] > flip_list[j][1]) {
                        // Swap
                        uint8_t temp_r = flip_list[i][0];
                        uint8_t temp_c = flip_list[i][1];
                        flip_list[i][0] = flip_list[j][0];
                        flip_list[i][1] = flip_list[j][1];
                        flip_list[j][0] = temp_r;
                        flip_list[j][1] = temp_c;
                    }
                }
            }
            break;
            
        case SWEEP_RANDOM:
            // Simple Fisher-Yates shuffle
            for (uint16_t i = flip_count - 1; i > 0; i--) {
                uint16_t j = rand() % (i + 1);
                // Swap
                uint8_t temp_r = flip_list[i][0];
                uint8_t temp_c = flip_list[i][1];
                flip_list[i][0] = flip_list[j][0];
                flip_list[i][1] = flip_list[j][1];
                flip_list[j][0] = temp_r;
                flip_list[j][1] = temp_c;
            }
            break;
            
        case SWEEP_DIAG:
            // Not implemented yet
            break;
    }
    
    // Set pixels according to the flip list
    for (uint16_t i = 0; i < flip_count; i++) {
        uint8_t r = flip_list[i][0];
        uint8_t c = flip_list[i][1];
        flip_dot_set_pixel(display, r, c, data[r][c]);
    }
    
    // Update display state
    memcpy(display->pixel_state, data, sizeof(display->pixel_state));
}

void flip_dot_set_rows_cols(flip_dot_t *display, uint8_t row_start, uint8_t row_end, uint8_t col_start, uint8_t col_end, bool pixel_value) {
    for (uint8_t r = row_start; r <= row_end; r++) {
        for (uint8_t c = col_start; c <= col_end; c++) {
            flip_dot_set_pixel(display, r, c, pixel_value);
        }
    }
}
