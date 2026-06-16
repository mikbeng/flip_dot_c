/**
 * @file clock.h
 * @brief MM:SS clock application for flip dot display
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "flip_dot.h"

typedef bool (*clock_app_abort_cb_t)(void);

/**
 * Run the clock until should_abort returns true (or NULL to run forever).
 * Renders MM:SS centered on the display and refreshes once per second.
 * Counts up from 00:00 while the mode is active.
 */
void clock_app_run(flip_dot_t *display, clock_app_abort_cb_t should_abort);

#endif /* CLOCK_H */
