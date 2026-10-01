"""Procedural animation frame generators."""

from generators.bouncing_ball import generate as bouncing_ball
from generators.game_of_life import generate as game_of_life
from generators.matrix_rain import generate as matrix_rain
from generators.ripple import generate as ripple
from generators.scrolling_text import generate as scrolling_text
from generators.sine_wave import generate as sine_wave

GENERATORS = {
    "bouncing_ball": bouncing_ball,
    "sine_wave": sine_wave,
    "matrix_rain": matrix_rain,
    "ripple": ripple,
    "scrolling_text": scrolling_text,
    "game_of_life": game_of_life,
}
