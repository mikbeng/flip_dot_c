/**
 * @file anim.h
 * @brief Flash-resident flipbook animation player
 */

#ifndef ANIM_H
#define ANIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "flip_dot.h"
#include "animations_gen.h"

typedef bool (*flip_dot_abort_cb_t)(void);

bool anim_play(flip_dot_t *display, anim_id_t id, flip_dot_abort_cb_t should_abort);
bool anim_playlist_run(flip_dot_t *display, const anim_id_t *list, size_t count,
                       flip_dot_abort_cb_t should_abort);

#endif /* ANIM_H */
