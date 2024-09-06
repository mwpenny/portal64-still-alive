########################
## Texture conversion ##
########################

# All textures used from the original game
set(TEXTURES
    materials/concrete/concrete_modular_ceiling001a
    materials/concrete/concrete_modular_floor001a
    materials/concrete/concrete_modular_wall001b
    materials/concrete/concrete_modular_wall001d
    materials/concrete/observationwall_001a
    materials/decals/orange_spot
    materials/effects/brightglow_y
    materials/effects/fleck_ash2
    materials/effects/light_rail_beam1
    materials/effects/portal_1_particle
    materials/effects/portal_2_particle
    materials/effects/portal_cleanser
    materials/gamepadui/portal_logo
    materials/glass/glasswindow_frosted_002
    materials/glass/refract01_dx8
    materials/metal/metalgrate018
    materials/metal/metal_modular_floor001
    materials/metal/metalwall048b
    materials/metal/metalwall_bts_006a
    materials/models/player/chell_body
    materials/models/player/chell_face
    materials/models/props/autoportal_frame/autoportal_frame
    materials/models/props/ball_catcher_sheet
    materials/models/props/bed01
    materials/models/props/button
    materials/models/props/combine_ball_launcher
    materials/models/props/door_01
    materials/models/props/door_02
    materials/models/props/elevatorshaft_wall/elevatorshaft_wall
    materials/models/props/elevator_caps/elevator_caps
    materials/models/props/lab_monitor/lab_monitor_screen_text
    materials/models/props/light_rail_endcap
    materials/models/props/metal_box
    materials/models/props/portal_cleanser
    materials/models/props/radio
    materials/models/props/round_elevator_sheet_1
    materials/models/props/round_elevator_sheet_2
    materials/models/props/round_elevator_sheet_3
    materials/models/props/security_camera
    materials/models/props/toilet
    materials/models/props_animsigns/awe_total
    materials/models/props_animsigns/newsignage_back02
    materials/models/props_c17/door01a_skin6
    materials/models/weapons/v_models/v_portalgun/v_portalgun
    materials/nature/hazard_liquid
    materials/plastic/plasticwall003a
    materials/plastic/plasticwall004a
    materials/signage/clock/clock_dots
    materials/signage/clock/countdown
    materials/signage/hazard_orange_03b
    materials/signage/indicator_lights/indicator_lights_corner_floor
    materials/signage/indicator_lights/indicator_lights_floor
    materials/signage/overlay_aperture_logo_worn
    materials/signage/signage_doorstate
    materials/signage/signage_exit
    materials/signage/signage_overlay_arrow
    materials/signage/signage_overlay_boxdispenser
    materials/signage/signage_overlay_boxhurt
    materials/signage/signage_overlay_catcher
    materials/signage/signage_overlay_dots1
    materials/signage/signage_overlay_dots2
    materials/signage/signage_overlay_dots3
    materials/signage/signage_overlay_dots4
    materials/signage/signage_overlay_energyball
    materials/signage/signage_overlay_fling1
    materials/signage/signage_overlay_fling2
    materials/signage/signage_overlay_fountain
    materials/signage/signage_overlay_midair1
    materials/signage/signage_overlay_midair2
    materials/signage/signage_overlay_toxic
    materials/sprites/grav_flare
    materials/sprites/hud/portal_crosshairs
    materials/tile/observation_tilefloor001a
    materials/vgui/chapters/chapter1
    materials/vgui/chapters/chapter2
    materials/vgui/chapters/chapter3
    materials/vgui/chapters/chapter4
    materials/vgui/chapters/chapter5
    materials/vgui/chapters/chapter6
    materials/vgui/chapters/chapter7
    materials/vgui/chapters/chapter8
    materials/vgui/chapters/chapter9
    materials/vgui/chapters/chapter10
    materials/vgui/chapters/chapter11
)

function(_add_texture_convert_command INPUT_FILE OUTPUT_FILE)
    set(INPUT_FILE "${PAK_DIR}/${INPUT_FILE}.vtf")
    set(OUTPUT_FILE "${PAK_DIR}/${OUTPUT_FILE}.png")
    set(ARGS "${ARGN}")

    if(NOT ARGS)
        get_source_file_property(FILE_ARGS ${INPUT_FILE} ARGS)
        if (FILE_ARGS)
            set(ARGS "${FILE_ARGS}")
        endif()
    endif()

    cmake_path(
        RELATIVE_PATH INPUT_FILE
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_INPUT_FILE
    )

    add_custom_command(
        DEPENDS
            extract_vpks
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${VTF2PNG} ${ARGS} ${INPUT_FILE} ${OUTPUT_FILE}
        COMMENT
            "Converting ${RELATIVE_INPUT_FILE}"
        VERBATIM
    )
endfunction()

# Add commands for converting VTF textures to PNG
# There are some special cases for VTFs with multiple frames

set_property(SOURCE ${PAK_DIR}/materials/signage/clock/countdown.vtf
    PROPERTY ARGS
        -f 60
)

foreach(i RANGE 1 9)
    math(EXPR FRAME "60 - ${i}")

    _add_texture_convert_command(
        materials/signage/clock/countdown
        materials/signage/clock/countdown_${i}
        -f ${FRAME}
    )
endforeach()

_add_texture_convert_command(
    materials/signage/signage_doorstate
    materials/signage/signage_doorstate_on
    -f 2
)
_add_texture_convert_command(
    materials/signage/indicator_lights/indicator_lights_corner_floor
    materials/signage/indicator_lights/indicator_lights_corner_floor_on
    -f 2
)
_add_texture_convert_command(
    materials/signage/indicator_lights/indicator_lights_floor
    materials/signage/indicator_lights/indicator_lights_floor_on
    -f 2
)

foreach(TEXTURE ${TEXTURES})
    _add_texture_convert_command(${TEXTURE} ${TEXTURE})
endforeach()

############################
## Texture transformation ##
############################

# Adding a new .ims implies a new texture, which necessitates updating the
# TEXTURES list above, which will retrigger CMake on build and so the typical
# glob pitfalls do not apply here.
file(GLOB_RECURSE TEXTURE_SCRIPTS "materials/*.ims")

function(_get_texture_script_file_list TEXTURE_SCRIPT PROPERTY_NAME PARENT_DIR OUTPUT)
    get_source_file_property(FILE_LIST ${TEXTURE_SCRIPT} ${PROPERTY_NAME})

    if (FILE_LIST)
        list(TRANSFORM FILE_LIST PREPEND "${PARENT_DIR}/" OUTPUT_VARIABLE FILE_LIST)
        list(TRANSFORM FILE_LIST APPEND ".png" OUTPUT_VARIABLE FILE_LIST)
        list(APPEND ${OUTPUT} "${FILE_LIST}")
    endif()

    return(PROPAGATE ${OUTPUT})
endfunction()

function(_add_texture_transform_command TEXTURE_SCRIPT OUTPUT_LIST)
    cmake_path(
        RELATIVE_PATH TEXTURE_SCRIPT
        BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE TEXTURE_NAME
    )
    cmake_path(
        REMOVE_EXTENSION TEXTURE_NAME
        OUTPUT_VARIABLE TEXTURE_NAME
    )

    set(INPUT_FILE "${PAK_DIR}/${TEXTURE_NAME}.png")
    set(OUTPUT_FILE "${PAK_MODIFIED_DIR}/${TEXTURE_NAME}.png")

    set(DEPENDENCY_FILES ${INPUT_FILE} ${TEXTURE_SCRIPT})
    _get_texture_script_file_list(${TEXTURE_SCRIPT} ADDITIONAL_DEPS ${PAK_DIR} DEPENDENCY_FILES)

    set(OUTPUT_FILES ${OUTPUT_FILE})
    _get_texture_script_file_list(${TEXTURE_SCRIPT} ADDITIONAL_OUTPUTS ${PAK_MODIFIED_DIR} OUTPUT_FILES)

    cmake_path(
        RELATIVE_PATH INPUT_FILE
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_INPUT_FILE
    )

    add_custom_command(
        DEPENDS
            ${CONVERT_ASSET} ${DEPENDENCY_FILES}
        OUTPUT
            ${OUTPUT_FILES}
        COMMAND
            ${PYTHON3} ${CONVERT_ASSET} ${IMAGEMAGICK_CONVERT} ${INPUT_FILE} ${TEXTURE_SCRIPT} ${OUTPUT_FILE}
        WORKING_DIRECTORY
            # TODO: Change this to PAK_MODIFIED_DIR and update .ims files with relative path
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Transforming ${RELATIVE_INPUT_FILE}"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} "${OUTPUT_FILES}")
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for transforming converted PNG textures (crop, scale, etc.).
# There are some special cases due to some conversion scripts reading textures
# other than those they are named after (combining multiple textures), or
# producing more than one output (texture splitting).

set_property(SOURCE materials/effects/portal_cleanser.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/effects/spark
)
set_property(SOURCE materials/models/player/chell_body.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/player/chell_back_logo
        materials/models/player/chell_pants
)
set_property(SOURCE materials/models/player/chell_face.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/player/chell_head
)
set_property(SOURCE materials/models/props_animsigns/awe_total.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/props_animsigns/awe_total_copy_0
        materials/models/props_animsigns/awe_total_copy_1
        materials/models/props_animsigns/awe_total_copy_2
)
set_property(SOURCE materials/models/props/ball_catcher_sheet.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/props/ball_catcher_back
)
set_property(SOURCE materials/models/props/bed01.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/props/bed01_screen
)
set_property(SOURCE materials/models/props/button.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/props/button_base
)
set_property(SOURCE materials/models/props/round_elevator_sheet_1.ims
    PROPERTY ADDITIONAL_DEPS
        materials/models/props/round_elevator_sheet_3
)
set_property(SOURCE materials/models/props/round_elevator_sheet_1.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/props/round_elevator_floor
)
set_property(SOURCE materials/models/props/round_elevator_sheet_3.ims
    PROPERTY ADDITIONAL_OUTPUTS
        materials/models/props/elevator_sign
)
set_property(SOURCE materials/signage/clock/countdown.ims
    PROPERTY ADDITIONAL_DEPS
        materials/signage/clock/countdown_1
        materials/signage/clock/countdown_2
        materials/signage/clock/countdown_3
        materials/signage/clock/countdown_4
        materials/signage/clock/countdown_5
        materials/signage/clock/countdown_6
        materials/signage/clock/countdown_7
        materials/signage/clock/countdown_8
        materials/signage/clock/countdown_9
        materials/signage/clock/clock_dots
)
set_property(SOURCE materials/signage/indicator_lights/indicator_lights_floor.ims
    PROPERTY ADDITIONAL_DEPS
        materials/signage/indicator_lights/indicator_lights_corner_floor
)
set_property(SOURCE materials/signage/indicator_lights/indicator_lights_floor_on.ims
    PROPERTY ADDITIONAL_DEPS
        materials/signage/indicator_lights/indicator_lights_corner_floor_on
)
set_property(SOURCE materials/signage/signage_exit.ims
    PROPERTY ADDITIONAL_DEPS
        materials/signage/signage_overlay_arrow
        materials/signage/signage_overlay_boxdispenser
        materials/signage/signage_overlay_boxhurt
        materials/signage/signage_overlay_energyball
        materials/signage/signage_overlay_catcher
        materials/signage/signage_overlay_toxic
        materials/signage/signage_overlay_fountain
)
set_property(SOURCE materials/signage/signage_overlay_fling1.ims
    PROPERTY ADDITIONAL_DEPS
        materials/signage/signage_overlay_fling2
        materials/signage/signage_overlay_dots1
        materials/signage/signage_overlay_dots2
        materials/signage/signage_overlay_dots3
        materials/signage/signage_overlay_dots4
        materials/signage/signage_overlay_toxic
        materials/signage/signage_overlay_fountain
)
set_property(SOURCE materials/signage/signage_overlay_midair1.ims
    PROPERTY ADDITIONAL_DEPS
        materials/signage/signage_overlay_midair2
)

set(TEXTURES_TRANSFORMED "")

foreach(TEXTURE_SCRIPT ${TEXTURE_SCRIPTS})
    _add_texture_transform_command(${TEXTURE_SCRIPT} TEXTURES_TRANSFORMED)
endforeach()

add_custom_target(
    all_textures
    DEPENDS ${TEXTURES_TRANSFORMED}
)
