# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "pwr_ctrl.c" "flip_dot.c" "applications/snake/snake.c" "input.c" "input_espnow.c" "switch_input.c")
set(COMPONENT_ADD_INCLUDEDIRS "." "applications/snake")

register_component()
