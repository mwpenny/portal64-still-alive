set(PAK_MATERIALS_DIR "${PAK_DIR}/materials")
set(PAK_MODIFIED_MATERIALS_DIR "${PAK_MODIFIED_DIR}/materials")

########################
## Texture conversion ##
########################

# All textures used from the original game
set(TEXTURES
    concrete/concrete_modular_ceiling001a
    concrete/concrete_modular_floor001a
    concrete/concrete_modular_wall001b
    concrete/concrete_modular_wall001d
    concrete/observationwall_001a
    decals/orange_spot
    effects/brightglow_y
    effects/fleck_ash2
    effects/light_rail_beam1
    effects/portal_1_particle
    effects/portal_2_particle
    effects/portal_cleanser
    gamepadui/portal_logo
    glass/glasswindow_frosted_002
    glass/refract01_dx8
    metal/metalgrate018
    metal/metal_modular_floor001
    metal/metalwall048b
    metal/metalwall_bts_006a
    models/player/chell_body
    models/player/chell_face
    models/props/autoportal_frame/autoportal_frame
    models/props/ball_catcher_sheet
    models/props/bed01
    models/props/button
    models/props/combine_ball_launcher
    models/props/door_01
    models/props/door_02
    models/props/elevatorshaft_wall/elevatorshaft_wall
    models/props/elevator_caps/elevator_caps
    models/props/lab_monitor/lab_monitor_screen_text
    models/props/light_rail_endcap
    models/props/metal_box
    models/props/portal_cleanser
    models/props/radio
    models/props/round_elevator_sheet_1
    models/props/round_elevator_sheet_2
    models/props/round_elevator_sheet_3
    models/props/security_camera
    models/props/toilet
    models/props_animsigns/awe_total
    models/props_animsigns/newsignage_back02
    models/props_c17/door01a_skin6
    models/weapons/v_models/v_portalgun/v_portalgun
    nature/hazard_liquid
    plastic/plasticwall003a
    plastic/plasticwall004a
    signage/clock/clock_dots
    signage/clock/countdown
    signage/hazard_orange_03b
    signage/indicator_lights/indicator_lights_corner_floor
    signage/indicator_lights/indicator_lights_floor
    signage/overlay_aperture_logo_worn
    signage/signage_doorstate
    signage/signage_exit
    signage/signage_overlay_arrow
    signage/signage_overlay_boxdispenser
    signage/signage_overlay_boxhurt
    signage/signage_overlay_catcher
    signage/signage_overlay_dots1
    signage/signage_overlay_dots2
    signage/signage_overlay_dots3
    signage/signage_overlay_dots4
    signage/signage_overlay_energyball
    signage/signage_overlay_fling1
    signage/signage_overlay_fling2
    signage/signage_overlay_fountain
    signage/signage_overlay_midair1
    signage/signage_overlay_midair2
    signage/signage_overlay_toxic
    sprites/grav_flare
    sprites/hud/portal_crosshairs
    tile/observation_tilefloor001a
    vgui/chapters/chapter1
    vgui/chapters/chapter2
    vgui/chapters/chapter3
    vgui/chapters/chapter4
    vgui/chapters/chapter5
    vgui/chapters/chapter6
    vgui/chapters/chapter7
    vgui/chapters/chapter8
    vgui/chapters/chapter9
    vgui/chapters/chapter10
    vgui/chapters/chapter11
)

function(_add_texture_convert_command INPUT_FILE OUTPUT_FILE)
    set(INPUT_FILE "${PAK_MATERIALS_DIR}/${INPUT_FILE}.vtf")
    set(OUTPUT_FILE "${PAK_MATERIALS_DIR}/${OUTPUT_FILE}.png")
    set(ARGS "${ARGN}")

    if (NOT ARGS)
        get_source_file_property(FILE_ARGS ${INPUT_FILE} ARGS)
        if (FILE_ARGS)
            set(ARGS "${FILE_ARGS}")
        endif()
    endif()

    add_custom_command(
        DEPENDS
            extract_vpks ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${VTF2PNG_EXECUTABLE} ${ARGS} ${INPUT_FILE} ${OUTPUT_FILE}
        COMMENT
            "Converting $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

# Add commands for converting VTF textures to PNG
# There are some special cases for VTFs with multiple frames

set_property(SOURCE ${PAK_MATERIALS_DIR}/signage/clock/countdown.vtf
    PROPERTY ARGS
        -f 60
)

foreach(i RANGE 1 9)
    math(EXPR FRAME "60 - ${i}")

    _add_texture_convert_command(
        signage/clock/countdown
        signage/clock/countdown_${i}
        -f ${FRAME}
    )
endforeach()

_add_texture_convert_command(
    signage/signage_doorstate
    signage/signage_doorstate_on
    -f 2
)
_add_texture_convert_command(
    signage/indicator_lights/indicator_lights_corner_floor
    signage/indicator_lights/indicator_lights_corner_floor_on
    -f 2
)
_add_texture_convert_command(
    signage/indicator_lights/indicator_lights_floor
    signage/indicator_lights/indicator_lights_floor_on
    -f 2
)

foreach(TEXTURE ${TEXTURES})
    _add_texture_convert_command(${TEXTURE} ${TEXTURE})
endforeach()

################
## Valve logo ##
################

function(_add_extract_frame_command INPUT_FILE OUTPUT_FILE SECONDS)
    cmake_path(
        GET OUTPUT_FILE PARENT_PATH
        OUTPUT_DIR
    )

    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${FFmpeg_EXECUTABLE}
            -ss 00:00:${SECONDS}
            -i ${INPUT_FILE}
            -frames:v 1
            -update true
            -q:v 2
            -loglevel error
            -y ${OUTPUT_FILE}
        COMMAND
            ${ImageMagickConvert_EXECUTABLE}
            ${OUTPUT_FILE} -crop 491x369+265+202 -resize 160x120 ${OUTPUT_FILE}
        COMMENT
            "Extracting frame from $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

_add_extract_frame_command(
    ${VALVE_INTRO_VIDEO}
    ${PAK_MODIFIED_DIR}/images/valve.png
    4
)

_add_extract_frame_command(
    ${VALVE_INTRO_VIDEO}
    ${PAK_MODIFIED_DIR}/images/valve-no-logo.png
    1
)

############################
## Texture transformation ##
############################

# Adding a new .ims implies a new texture, which necessitates updating the
# TEXTURES list above, which will retrigger CMake on build and so the typical
# glob pitfalls do not apply here.
file(GLOB_RECURSE TEXTURE_SCRIPTS "*.ims")

function(_get_texture_script_file_list TEXTURE_SCRIPT PROPERTY_NAME PARENT_DIR OUTPUT)
    get_source_file_property(FILE_LIST ${TEXTURE_SCRIPT} ${PROPERTY_NAME})

    if (FILE_LIST)
        list(TRANSFORM FILE_LIST PREPEND "${PARENT_DIR}/" OUTPUT_VARIABLE FILE_LIST)
        list(TRANSFORM FILE_LIST APPEND ".png" OUTPUT_VARIABLE FILE_LIST)
        list(APPEND ${OUTPUT} ${FILE_LIST})
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

    set(INPUT_FILE "${PAK_MATERIALS_DIR}/${TEXTURE_NAME}.png")
    set(OUTPUT_FILE "${PAK_MODIFIED_MATERIALS_DIR}/${TEXTURE_NAME}.png")

    cmake_path(
        GET OUTPUT_FILE PARENT_PATH
        OUTPUT_DIR
    )

    set(DEPENDENCY_FILES ${INPUT_FILE} ${TEXTURE_SCRIPT})
    _get_texture_script_file_list(${TEXTURE_SCRIPT} ADDITIONAL_DEPS ${PAK_MATERIALS_DIR} DEPENDENCY_FILES)

    set(OUTPUT_FILES ${OUTPUT_FILE})
    _get_texture_script_file_list(${TEXTURE_SCRIPT} ADDITIONAL_OUTPUTS ${PAK_MODIFIED_MATERIALS_DIR} OUTPUT_FILES)

    add_custom_command(
        DEPENDS
            ${CONVERT_ASSET} ${DEPENDENCY_FILES}
        OUTPUT
            ${OUTPUT_FILES}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${Python3_EXECUTABLE} ${CONVERT_ASSET}
            ${ImageMagickConvert_EXECUTABLE}
            ${INPUT_FILE}
            ${TEXTURE_SCRIPT}
            ${OUTPUT_FILE}
        WORKING_DIRECTORY
            # TODO: Change this to PAK_MODIFIED_MATERIALS_DIR and update .ims files with relative path
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILES})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for transforming converted PNG textures (crop, scale, etc.).
# There are some special cases due to some conversion scripts reading textures
# other than those they are named after (combining multiple textures), or
# producing more than one output (texture splitting).

set_property(SOURCE effects/portal_cleanser.ims
    PROPERTY ADDITIONAL_OUTPUTS
        effects/spark
)
set_property(SOURCE models/player/chell_body.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/player/chell_back_logo
        models/player/chell_pants
)
set_property(SOURCE models/player/chell_face.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/player/chell_head
)
set_property(SOURCE models/props_animsigns/awe_total.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props_animsigns/awe_total_copy_0
        models/props_animsigns/awe_total_copy_1
        models/props_animsigns/awe_total_copy_2
)
set_property(SOURCE models/props/ball_catcher_sheet.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/ball_catcher_back
)
set_property(SOURCE models/props/bed01.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/bed01_screen
)
set_property(SOURCE models/props/button.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/button_base
)
set_property(SOURCE models/props/round_elevator_sheet_1.ims
    PROPERTY ADDITIONAL_DEPS
        models/props/round_elevator_sheet_3
)
set_property(SOURCE models/props/round_elevator_sheet_1.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/round_elevator_floor
)
set_property(SOURCE models/props/round_elevator_sheet_3.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/elevator_sign
)
set_property(SOURCE signage/clock/countdown.ims
    PROPERTY ADDITIONAL_DEPS
        signage/clock/countdown_1
        signage/clock/countdown_2
        signage/clock/countdown_3
        signage/clock/countdown_4
        signage/clock/countdown_5
        signage/clock/countdown_6
        signage/clock/countdown_7
        signage/clock/countdown_8
        signage/clock/countdown_9
        signage/clock/clock_dots
)
set_property(SOURCE signage/indicator_lights/indicator_lights_floor.ims
    PROPERTY ADDITIONAL_DEPS
        signage/indicator_lights/indicator_lights_corner_floor
)
set_property(SOURCE signage/indicator_lights/indicator_lights_floor_on.ims
    PROPERTY ADDITIONAL_DEPS
        signage/indicator_lights/indicator_lights_corner_floor_on
)
set_property(SOURCE signage/signage_exit.ims
    PROPERTY ADDITIONAL_DEPS
        signage/signage_overlay_arrow
        signage/signage_overlay_boxdispenser
        signage/signage_overlay_boxhurt
        signage/signage_overlay_energyball
        signage/signage_overlay_catcher
        signage/signage_overlay_toxic
        signage/signage_overlay_fountain
)
set_property(SOURCE signage/signage_overlay_fling1.ims
    PROPERTY ADDITIONAL_DEPS
        signage/signage_overlay_fling2
        signage/signage_overlay_dots1
        signage/signage_overlay_dots2
        signage/signage_overlay_dots3
        signage/signage_overlay_dots4
        signage/signage_overlay_toxic
        signage/signage_overlay_fountain
)
set_property(SOURCE signage/signage_overlay_midair1.ims
    PROPERTY ADDITIONAL_DEPS
        signage/signage_overlay_midair2
)

set(TEXTURES_TRANSFORMED "")

foreach(TEXTURE_SCRIPT ${TEXTURE_SCRIPTS})
    _add_texture_transform_command(${TEXTURE_SCRIPT} TEXTURES_TRANSFORMED)
endforeach()

add_custom_target(textures
    DEPENDS ${TEXTURES_TRANSFORMED}
)
set_target_properties(textures PROPERTIES
    # Enable dependents to easily depend on transformed textures
    OUTPUTS ${TEXTURES_TRANSFORMED}
)

#########################
## Material generation ##
#########################

set(CORE_MATERIALS
    hud.skm.yaml
    static.skm.yaml
    ui.skm.yaml
)

set(IMAGE_MATERIALS
    images.skm.yaml
)

function(_add_material_generate_command INPUT_FILE OUTPUT_LIST)
    cmake_path(
        GET INPUT_FILE STEM
        MATERIAL_NAME
    )
    cmake_path(
        RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_DIR
    )

    set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
    set(OUTPUT_FILE_H "${PROJECT_BINARY_DIR}/${RELATIVE_DIR}/${MATERIAL_NAME}.h")
    set(OUTPUT_FILE_C "${PROJECT_BINARY_DIR}/${RELATIVE_DIR}/${MATERIAL_NAME}_mat.c")

    # Depend on all textures for simplicity
    set(DEPENDENCY_FILES ${INPUT_FILE} ${TEXTURES_TRANSFORMED})

    get_source_file_property(DEP_FILE_LIST ${INPUT_FILE} ADDITIONAL_DEPS)
    if (DEP_FILE_LIST)
        list(APPEND DEPENDENCY_FILES ${DEP_FILE_LIST})
    endif()

    add_custom_command(
        DEPENDS
            textures ${DEPENDENCY_FILES}
            skeletool64::skeletool64
        OUTPUT
            ${OUTPUT_FILE_H} ${OUTPUT_FILE_C}
        COMMAND
            skeletool64::skeletool64
            --name ${MATERIAL_NAME}
            -m ${INPUT_FILE}
            --material-output
            --output ${OUTPUT_FILE_H}
        COMMENT
            "Generating materials for $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE_H} ${OUTPUT_FILE_C})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for generating materials
# There are some special cases for materials which use non-VPK textures

set_property(SOURCE hud.skm.yaml
    PROPERTY ADDITIONAL_DEPS
        ${PROJECT_SOURCE_DIR}/assets/images/center_reticle.png
)

set_property(SOURCE images.skm.yaml
    PROPERTY ADDITIONAL_DEPS
        ${PAK_MODIFIED_DIR}/images/valve.png
)

set_property(SOURCE ui.skm.yaml
    PROPERTY ADDITIONAL_DEPS
        ${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_0.png
        ${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_1.png
        ${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_2.png
        ${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_3.png
        ${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_4.png
        ${PROJECT_SOURCE_DIR}/assets/fonts/liberation_mono/liberation_mono_0.png
        ${PROJECT_SOURCE_DIR}/assets/images/button_icons.png
        ${PROJECT_SOURCE_DIR}/assets/images/credits_icons.png
        ${PROJECT_SOURCE_DIR}/assets/images/github_qr.png
        ${PROJECT_SOURCE_DIR}/assets/images/rounded_corners.png
)

set(GENERATED_CORE_MATERIALS "")
set(GENERATED_IMAGE_MATERIALS "")

foreach(MATERIAL ${CORE_MATERIALS})
    _add_material_generate_command(${MATERIAL} GENERATED_CORE_MATERIALS)
endforeach()

foreach(MATERIAL ${IMAGE_MATERIALS})
    _add_material_generate_command(${MATERIAL} GENERATED_IMAGE_MATERIALS)
endforeach()

add_custom_target(materials
    DEPENDS
        ${GENERATED_CORE_MATERIALS}
        ${GENERATED_IMAGE_MATERIALS}
)

add_library(materials_core INTERFACE)
add_dependencies(materials_core materials)
target_sources(materials_core INTERFACE
    ${GENERATED_CORE_MATERIALS}
)

add_library(materials_images INTERFACE)
add_dependencies(materials_images materials)
target_sources(materials_images INTERFACE
    ${GENERATED_IMAGE_MATERIALS}
)
