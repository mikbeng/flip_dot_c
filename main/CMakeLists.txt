# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(FLIP_ANIM_DIR "${CMAKE_SOURCE_DIR}/tools/flip_anim")
set(ANIM_GEN_C "${CMAKE_CURRENT_BINARY_DIR}/animations_gen.c")
set(ANIM_GEN_H "${CMAKE_CURRENT_BINARY_DIR}/animations_gen.h")

file(GLOB FLIP_ANIM_GENERATORS "${FLIP_ANIM_DIR}/generators/*.py")

add_custom_command(
    OUTPUT "${ANIM_GEN_C}" "${ANIM_GEN_H}"
    COMMAND "${Python3_EXECUTABLE}" "${FLIP_ANIM_DIR}/build_animations.py"
        --manifest "${FLIP_ANIM_DIR}/manifest.yaml"
        --out-c "${ANIM_GEN_C}"
        --out-h "${ANIM_GEN_H}"
    DEPENDS
        "${FLIP_ANIM_DIR}/build_animations.py"
        "${FLIP_ANIM_DIR}/emit_c.py"
        "${FLIP_ANIM_DIR}/format.py"
        "${FLIP_ANIM_DIR}/manifest.yaml"
        ${FLIP_ANIM_GENERATORS}
    WORKING_DIRECTORY "${FLIP_ANIM_DIR}"
    COMMENT "Generating flip-dot animation blobs"
    VERBATIM
)

add_custom_target(animations_gen_target DEPENDS "${ANIM_GEN_C}" "${ANIM_GEN_H}")

set(COMPONENT_SRCS
    "main.c"
    "pwr_ctrl.c"
    "flip_dot.c"
    "anim.c"
    "${ANIM_GEN_C}"
    "applications/snake/snake.c"
    "applications/clock/clock.c"
    "input.c"
    "input_espnow.c"
    "switch_input.c"
)
set(COMPONENT_ADD_INCLUDEDIRS "." "applications/snake" "applications/clock" "${CMAKE_CURRENT_BINARY_DIR}")

register_component()

add_dependencies(${COMPONENT_LIB} animations_gen_target)
