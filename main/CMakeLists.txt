# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "pwr_ctrl.c" "flip_dot.c" "snake.c" "input.c" "input_espnow.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
