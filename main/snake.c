/**
 * @file snake.c
 * @brief Snake game application implementation
 *
 * @author Mikael Bengtsson
 * @date 2025-06-11
 */

#include "snake.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/******************************************************************************
 * Private Definitions and Types
 ******************************************************************************/

static const char *TAG = "snake_game";

/******************************************************************************
 * Private Function Declarations
 ******************************************************************************/

// Internal helper functions
static void init_snake(snake_t *snake);
static void move_snake(snake_t *snake);
static bool is_valid_position(position_t pos);
static bool is_position_in_snake(snake_t *snake, position_t pos);
static void clear_game_buffer(uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH]);
static uint32_t get_random_number(uint32_t max);

/******************************************************************************
 * Private Function Implementations
 ******************************************************************************/

static void init_snake(snake_t *snake) {
    snake->length = SNAKE_INITIAL_LENGTH;
    snake->direction = DIR_RIGHT;
    snake->next_direction = DIR_RIGHT;
    
    // Initialize snake in the center of the play area
    uint8_t start_x = (GAME_MIN_X + GAME_MAX_X) / 2;
    uint8_t start_y = (GAME_MIN_Y + GAME_MAX_Y) / 2;
    
    for (uint8_t i = 0; i < snake->length; i++) {
        snake->segments[i].x = start_x - i;
        snake->segments[i].y = start_y;
    }
}

static void move_snake(snake_t *snake) {
    // Update direction if a direction change is buffered
    snake->direction = snake->next_direction;
    
    // Calculate new head position
    position_t new_head = snake->segments[0];
    
    switch (snake->direction) {
        case DIR_UP:
            new_head.y--;
            break;
        case DIR_DOWN:
            new_head.y++;
            break;
        case DIR_LEFT:
            new_head.x--;
            break;
        case DIR_RIGHT:
            new_head.x++;
            break;
    }
    
    // Move body segments
    for (int8_t i = snake->length - 1; i > 0; i--) {
        snake->segments[i] = snake->segments[i - 1];
    }
    
    // Set new head position
    snake->segments[0] = new_head;
}

static bool is_valid_position(position_t pos) {
    return (pos.x <= GAME_MAX_X && pos.y <= GAME_MAX_Y);
}

static bool is_position_in_snake(snake_t *snake, position_t pos) {
    for (uint8_t i = 0; i < snake->length; i++) {
        if (snake->segments[i].x == pos.x && snake->segments[i].y == pos.y) {
            return true;
        }
    }
    return false;
}



static void clear_game_buffer(uint8_t buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH]) {
    memset(buffer, 0, DISPLAY_HEIGHT * DISPLAY_WIDTH);
}

static uint32_t get_random_number(uint32_t max) {
    return rand() % max;
}

/******************************************************************************
 * Public Function Implementations
 ******************************************************************************/

void snake_game_init(snake_game_t *game, flip_dot_t *display) {
    ESP_LOGI(TAG, "Initializing Snake game");
    
    game->display = display;
    game->state = GAME_INIT;
    game->score = 0;
    game->level = 1;
    game->game_speed_ms = SNAKE_INITIAL_SPEED_MS;
    
    // Initialize game buffer
    clear_game_buffer(game->game_buffer);
    
    // Initialize snake
    init_snake(&game->snake);
    
    // Initialize food
    for (uint8_t i = 0; i < FOOD_COUNT; i++) {
        game->food[i].active = false;
    }
    
    // Generate initial food
    snake_game_generate_food(game);
    
    ESP_LOGI(TAG, "Snake game initialized");
}

void snake_game_reset(snake_game_t *game) {
    ESP_LOGI(TAG, "Resetting Snake game");
    
    game->score = 0;
    game->level = 1;
    game->game_speed_ms = SNAKE_INITIAL_SPEED_MS;
    game->state = GAME_INIT;
    
    // Reset snake
    init_snake(&game->snake);
    
    // Reset food
    for (uint8_t i = 0; i < FOOD_COUNT; i++) {
        game->food[i].active = false;
    }
    snake_game_generate_food(game);
    
    // Clear display
    clear_game_buffer(game->game_buffer);
    snake_game_render(game);
}

void snake_game_start(snake_game_t *game) {
    ESP_LOGI(TAG, "Starting Snake game");
    game->state = GAME_RUNNING;
}

void snake_game_pause(snake_game_t *game) {
    if (game->state == GAME_RUNNING) {
        ESP_LOGI(TAG, "Pausing Snake game");
        game->state = GAME_PAUSED;
    }
}

void snake_game_resume(snake_game_t *game) {
    if (game->state == GAME_PAUSED) {
        ESP_LOGI(TAG, "Resuming Snake game");
        game->state = GAME_RUNNING;
    }
}

void snake_game_update(snake_game_t *game) {
    if (game->state != GAME_RUNNING) {
        return;
    }
    
    // Move snake
    move_snake(&game->snake);
    
    // Check for collisions
    if (snake_game_check_collision(game)) {
        ESP_LOGI(TAG, "Game Over! Final score: %ld", game->score);
        game->state = GAME_OVER;
        return;
    }
    
    // Check for food collision
    if (snake_game_check_food_collision(game)) {
        ESP_LOGI(TAG, "Food eaten! Score: %ld", game->score);
        
        // Increase snake length
        if (game->snake.length < SNAKE_MAX_LENGTH) {
            game->snake.length++;
        }
        
        // Increase score
        game->score += 10;
        
        // Check for level up (every 50 points)
        if (game->score % 50 == 0) {
            game->level++;
            if (game->game_speed_ms > SNAKE_MIN_SPEED_MS) {
                game->game_speed_ms -= SNAKE_SPEED_DECREASE_PER_LEVEL;
                if (game->game_speed_ms < SNAKE_MIN_SPEED_MS) {
                    game->game_speed_ms = SNAKE_MIN_SPEED_MS;
                }
            }
            ESP_LOGI(TAG, "Level up! Level: %ld, Speed: %ld ms", game->level, game->game_speed_ms);
        }
        
        // Generate new food
        snake_game_generate_food(game);
    }
    
    // Render the game
    snake_game_render(game);
}

void snake_game_change_direction(snake_game_t *game, snake_direction_t new_direction) {
    // Prevent reversing into itself
    switch (game->snake.direction) {
        case DIR_UP:
            if (new_direction != DIR_DOWN) {
                game->snake.next_direction = new_direction;
            }
            break;
        case DIR_DOWN:
            if (new_direction != DIR_UP) {
                game->snake.next_direction = new_direction;
            }
            break;
        case DIR_LEFT:
            if (new_direction != DIR_RIGHT) {
                game->snake.next_direction = new_direction;
            }
            break;
        case DIR_RIGHT:
            if (new_direction != DIR_LEFT) {
                game->snake.next_direction = new_direction;
            }
            break;
    }
}

bool snake_game_is_running(snake_game_t *game) {
    return game->state == GAME_RUNNING;
}

bool snake_game_is_over(snake_game_t *game) {
    return game->state == GAME_OVER;
}

void snake_game_render(snake_game_t *game) {
    // Clear the game buffer
    clear_game_buffer(game->game_buffer);
    
    // Draw snake
    for (uint8_t i = 0; i < game->snake.length; i++) {
        position_t pos = game->snake.segments[i];
        if (pos.x < DISPLAY_WIDTH && pos.y < DISPLAY_HEIGHT) {
            game->game_buffer[pos.y][pos.x] = 1;
        }
    }
    
    // Draw food
    for (uint8_t i = 0; i < FOOD_COUNT; i++) {
        if (game->food[i].active) {
            position_t pos = game->food[i].position;
            if (pos.x < DISPLAY_WIDTH && pos.y < DISPLAY_HEIGHT) {
                game->game_buffer[pos.y][pos.x] = 1;
            }
        }
    }
    
    // Update the display
    flip_dot_update_display(game->display, game->game_buffer);
}

void snake_game_show_game_over(snake_game_t *game) {
    ESP_LOGI(TAG, "Showing game over screen");
    
    // Clear display
    clear_game_buffer(game->game_buffer);
    
    // Simple game over pattern - create a cross pattern
    for (uint8_t i = 2; i < DISPLAY_WIDTH - 2; i++) {
        game->game_buffer[DISPLAY_HEIGHT / 2][i] = 1;  // Horizontal line
    }
    for (uint8_t i = 2; i < DISPLAY_HEIGHT - 2; i++) {
        game->game_buffer[i][DISPLAY_WIDTH / 2] = 1;   // Vertical line
    }
    
    // Update display
    flip_dot_update_display(game->display, game->game_buffer);
    
    // Hold for a few seconds
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

void snake_game_show_score(snake_game_t *game) {
    ESP_LOGI(TAG, "Final Score: %ld, Level: %ld", game->score, game->level);
    // Score display could be implemented with a simple pattern or number display
    // For now, just log the score
}

void snake_game_generate_food(snake_game_t *game) {
    // Find inactive food slot
    uint8_t food_index = 0;
    for (uint8_t i = 0; i < FOOD_COUNT; i++) {
        if (!game->food[i].active) {
            food_index = i;
            break;
        }
    }
    
    // Generate random position that's not in the snake
    position_t new_pos;
    uint8_t attempts = 0;
    const uint8_t max_attempts = 100;
    
    do {
        new_pos.x = GAME_MIN_X + get_random_number(GAME_MAX_X - GAME_MIN_X + 1);
        new_pos.y = GAME_MIN_Y + get_random_number(GAME_MAX_Y - GAME_MIN_Y + 1);
        attempts++;
    } while (is_position_in_snake(&game->snake, new_pos) && attempts < max_attempts);
    
    if (attempts < max_attempts) {
        game->food[food_index].position = new_pos;
        game->food[food_index].active = true;
        ESP_LOGD(TAG, "Food generated at (%d, %d)", new_pos.x, new_pos.y);
    } else {
        ESP_LOGW(TAG, "Could not generate food after %d attempts", max_attempts);
    }
}

bool snake_game_check_collision(snake_game_t *game) {
    position_t head = game->snake.segments[0];
    
    // Check wall collision
    if (!is_valid_position(head)) {
        ESP_LOGD(TAG, "Wall collision at (%d, %d)", head.x, head.y);
        return true;
    }
    
    // Check self collision (skip head segment)
    for (uint8_t i = 1; i < game->snake.length; i++) {
        if (game->snake.segments[i].x == head.x && game->snake.segments[i].y == head.y) {
            ESP_LOGD(TAG, "Self collision at (%d, %d)", head.x, head.y);
            return true;
        }
    }
    
    return false;
}

bool snake_game_check_food_collision(snake_game_t *game) {
    position_t head = game->snake.segments[0];
    
    for (uint8_t i = 0; i < FOOD_COUNT; i++) {
        if (game->food[i].active &&
            game->food[i].position.x == head.x &&
            game->food[i].position.y == head.y) {
            
            // Deactivate the eaten food
            game->food[i].active = false;
            ESP_LOGD(TAG, "Food collision at (%d, %d)", head.x, head.y);
            return true;
        }
    }
    
    return false;
}

void snake_game_demo(flip_dot_t *display, uint32_t duration_ms) {
    ESP_LOGI(TAG, "Starting Snake game demo");
    
    snake_game_t game;
    snake_game_init(&game, display);
    snake_game_start(&game);
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t last_update = start_time;
    uint32_t direction_change_time = start_time;
    
    // Simple AI: change direction every few seconds
    snake_direction_t directions[] = {DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_UP};
    uint8_t current_dir_index = 0;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < duration_ms) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Change direction every 3 seconds for demo
        if (current_time - direction_change_time > 3000) {
            current_dir_index = (current_dir_index + 1) % 4;
            snake_game_change_direction(&game, directions[current_dir_index]);
            direction_change_time = current_time;
        }
        
        // Update game at the appropriate speed
        if (current_time - last_update >= game.game_speed_ms) {
            snake_game_update(&game);
            last_update = current_time;
            
            // Reset game if it's over
            if (snake_game_is_over(&game)) {
                snake_game_show_game_over(&game);
                snake_game_reset(&game);
                snake_game_start(&game);
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent busy waiting
    }
    
    ESP_LOGI(TAG, "Snake game demo completed");
} 