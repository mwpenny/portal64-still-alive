set(PAK_MATERIALS_DIR "${PAK_DIR}/materials")
set(PAK_MODIFIED_MATERIALS_DIR "${PAK_MODIFIED_DIR}/materials")

#####################
## Material export ##
#####################

set(TEXTURE_SCRIPTS
    concrete/concrete_bts_ceiling002a.ims
    concrete/concrete_bts_floor002a.ims
    concrete/concrete_bts_modular_wall001b.ims
    concrete/concrete_bts_modular_wall001c.ims
    concrete/concrete_modular_ceiling001a.ims
    concrete/concrete_modular_floor001a.ims
    concrete/concrete_modular_wall001b.ims
    concrete/concrete_modular_wall001d.ims
    concrete/observationwall_001a.ims
    decals/orange_spot.ims
    effects/brightglow_y.ims
    effects/fleck_ash2.ims
    effects/light_rail_beam1.ims
    effects/portal_1_particle.ims
    effects/portal_2_particle.ims
    effects/portal_cleanser.ims
    gamepadui/portal_logo.ims
    gamepadui/valve.ims
    glass/glasswindow_frosted_002.ims
    glass/refract01_dx8.ims
    metal/metalbeam_bts_001a.ims
    metal/metaldoor_white01.ims
    metal/metalfloor_bts_001b.ims
    metal/metalgrate018.ims
    metal/metal_modular_floor001.ims
    metal/metalvent014a.ims
    metal/metalwall048b.ims
    metal/metalwall060a.ims
    metal/metalwall061f.ims
    metal/metalwall_bts_005a.ims
    metal/metalwall_bts_006a.ims
    models/player/chell_body.ims
    models/player/chell_face.ims
    models/props/autoportal_frame/autoportal_frame.ims
    models/props/ball_catcher_sheet.ims
    models/props/bed01.ims
    models/props/button.ims
    models/props/combine_ball_launcher.ims
    models/props/door_01.ims
    models/props/door_02.ims
    models/props/elevator_caps/elevator_caps.ims
    models/props/elevatorshaft_wall/elevatorshaft_wall.ims
    models/props/food_can/food_can.ims
    models/props/lab_monitor/lab_monitor_screen_text.ims
    models/props/light_rail_endcap.ims
    models/props/metal_box.ims
    models/props/metal_box_skin001.ims
    models/props/milk_carton/milk_carton.ims
    models/props/pc_case_open/pc_case_open.ims
    models/props/portal_cleanser.ims
    models/props/radio.ims
    models/props/round_elevator_sheet_1.ims
    models/props/round_elevator_sheet_2.ims
    models/props/round_elevator_sheet_3.ims
    models/props/saucepan/saucepan.ims
    models/props/security_camera.ims
    models/props/toilet.ims
    models/props/water_bottle/water_bottle.ims
    models/props_animsigns/awe_total.ims
    models/props_animsigns/newsignage_back02.ims
    models/props_bts/fan01.ims
    models/props_bts/glados_aperturedoor.ims
    models/props_c17/door01a_skin6.ims
    models/props_c17/substation_transformer01b.ims
    models/props_junk/cardboard_boxes001a.ims
    models/props_junk/metalbucket01a.ims
    models/props_wasteland/fence_alpha.ims
    models/props_wasteland/prison_yard001.ims
    models/weapons/v_models/v_portalgun/v_portalgun.ims
    nature/hazard_liquid.ims
    overlays/aperture_lrg001.ims
    overlays/cake_picture.ims
    overlays/overlay_handprint.ims
    overlays/overlay_scrawlings002a.ims
    overlays/overlay_scrawlings003a.ims
    overlays/overlay_scrawlings004a.ims
    overlays/overlay_scrawlings005a.ims
    overlays/overlay_scrawlings006a.ims
    overlays/overlay_scrawlings006b.ims
    overlays/overlay_scrawlings006c.ims
    particle/smoke1/smoke1.ims
    plastic/plasticwall003a.ims
    plastic/plasticwall004a.ims
    signage/clock/countdown.ims
    signage/hazard_orange_03b.ims
    signage/indicator_lights/indicator_lights_floor.ims
    signage/indicator_lights/indicator_lights_floor_on.ims
    signage/overlay_aperture_logo_worn.ims
    signage/signage_doorstate.ims
    signage/signage_doorstate_on.ims
    signage/signage_exit.ims
    signage/signage_overlay_fling1.ims
    signage/signage_overlay_midair1.ims
    sprites/grav_flare.ims
    sprites/hud/portal_crosshairs.ims
    sprites/muzzleflash2.ims
    tile/observation_tilefloor001a.ims
    vgui/chapters/chapter1.ims
    vgui/chapters/chapter2.ims
    vgui/chapters/chapter3.ims
    vgui/chapters/chapter4.ims
    vgui/chapters/chapter5.ims
    vgui/chapters/chapter6.ims
    vgui/chapters/chapter7.ims
    vgui/chapters/chapter8.ims
    vgui/chapters/chapter9.ims
    vgui/chapters/chapter10.ims
    vgui/chapters/chapter11.ims
)

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
set_property(SOURCE models/props/milk_carton/milk_carton.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/milk_carton/milk_carton_bottom
)
set_property(SOURCE models/props/pc_case_open/pc_case_open.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/pc_case_open/pc_case_logo_side
        models/props/pc_case_open/pc_case_logo_front
        models/props/pc_case_open/pc_case_grill
        models/props/pc_case_open/pc_case_button
        models/props/pc_case_open/pc_case_bottom
        models/props/pc_case_open/pc_case_interior
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
set_property(SOURCE models/props/water_bottle/water_bottle.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props/water_bottle/water_bottle_label
)
set_property(SOURCE models/props_bts/glados_aperturedoor.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props_bts/glados_aperturedoor_top
)
set_property(SOURCE models/props_c17/substation_transformer01b.ims
    PROPERTY ADDITIONAL_OUTPUTS
        models/props_c17/substation_transformer01b_center
        models/props_c17/substation_transformer01b_rim
)
set_property(SOURCE overlays/overlay_scrawlings002a.ims
    PROPERTY ADDITIONAL_DEPS
        concrete/concrete_bts_modular_wall001b
)
set_property(SOURCE overlays/overlay_scrawlings006a.ims
    PROPERTY ADDITIONAL_DEPS
        concrete/concrete_bts_modular_wall001b
)
set_property(SOURCE overlays/overlay_scrawlings006b.ims
    PROPERTY ADDITIONAL_DEPS
        concrete/concrete_bts_modular_wall001b
)
set_property(SOURCE overlays/overlay_scrawlings006c.ims
    PROPERTY ADDITIONAL_DEPS
        concrete/concrete_bts_modular_wall001b
)
foreach(i RANGE 2 32)
    set_property(SOURCE overlays/overlay_scrawlings002a.ims
        APPEND
        PROPERTY ADDITIONAL_OUTPUTS
            overlays/overlay_scrawlings002a_${i}
    )
    set_property(SOURCE overlays/overlay_scrawlings006a.ims
        APPEND
        PROPERTY ADDITIONAL_OUTPUTS
            overlays/overlay_scrawlings006a_${i}
    )
    set_property(SOURCE overlays/overlay_scrawlings006b.ims
        APPEND
        PROPERTY ADDITIONAL_OUTPUTS
            overlays/overlay_scrawlings006b_${i}
    )
    set_property(SOURCE overlays/overlay_scrawlings006c.ims
        APPEND
        PROPERTY ADDITIONAL_OUTPUTS
            overlays/overlay_scrawlings006c_${i}
    )
endforeach()
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
        signage/signage_overlay_companioncube
        signage/signage_overlay_companioncube_broken
        signage/signage_overlay_incinerator
        signage/signage_overlay_midair2
        signage/signage_overlay_turret
)

function(_get_property_image_files FILE_PATH PROPERTY_NAME PARENT_DIR OUTPUT)
    get_source_file_property(FILE_LIST ${FILE_PATH} ${PROPERTY_NAME})

    if (FILE_LIST)
        list(TRANSFORM FILE_LIST PREPEND "${PARENT_DIR}/" OUTPUT_VARIABLE FILE_LIST)
        list(TRANSFORM FILE_LIST APPEND ".png" OUTPUT_VARIABLE FILE_LIST)
        list(APPEND ${OUTPUT} ${FILE_LIST})
    endif()

    return(PROPAGATE ${OUTPUT})
endfunction()

################
## Valve logo ##
################

function(_add_extract_frame_command INPUT_FILE OUTPUT_FILE SECONDS)
    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>"
        COMMAND
            ${FFmpeg_EXECUTABLE}
            -ss 00:00:${SECONDS}
            -i ${INPUT_FILE}
            -frames:v 1
            -update true
            -q:v 2
            -loglevel error
            -y ${OUTPUT_FILE}
        COMMENT
            "Extracting frame from $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

_add_extract_frame_command(
    ${VALVE_INTRO_VIDEO}
    ${PAK_MATERIALS_DIR}/gamepadui/valve.png
    4
)

_add_extract_frame_command(
    ${VALVE_INTRO_VIDEO}
    ${PAK_MATERIALS_DIR}/gamepadui/valve-no-logo.png
    1
)

########################
## Texture conversion ##
########################

function(_add_texture_convert_command INPUT_FILE OUTPUT_FILE OUTPUT_LIST)
    set(INPUT_FILE "${PAK_MATERIALS_DIR}/${INPUT_FILE}.vtf")
    set(OUTPUT_FILE "${PAK_MATERIALS_DIR}/${OUTPUT_FILE}.png")
    set(ARGS "${ARGN}")

    add_custom_command(
        DEPENDS
            extract_vpks "$<TARGET_PROPERTY:extract_vpks,OUTPUTS>"
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${VTF2PNG_EXECUTABLE} ${ARGS} ${INPUT_FILE} ${OUTPUT_FILE}
        COMMENT
            "Converting $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

function(_texture_has_convert_command TEXTURE_NAME HAS_COMMAND_OUT)
    set(CONVERTED_FILE "${PAK_MATERIALS_DIR}/${TEXTURE_NAME}.png")
    get_source_file_property(${HAS_COMMAND_OUT} ${CONVERTED_FILE} GENERATED)
    return(PROPAGATE ${HAS_COMMAND_OUT})
endfunction()

# Add commands for converting VTF textures to PNG
# There are some special cases for VTFs with multiple frames

set(TEXTURES_CONVERTED "")

_add_texture_convert_command(
    signage/clock/countdown
    signage/clock/countdown
    TEXTURES_CONVERTED
    -f 60
)
foreach(i RANGE 1 9)
    math(EXPR FRAME "60 - ${i}")

    _add_texture_convert_command(
        signage/clock/countdown
        signage/clock/countdown_${i}
        TEXTURES_CONVERTED
        -f ${FRAME}
    )
endforeach()

_add_texture_convert_command(
    signage/signage_doorstate
    signage/signage_doorstate_on
    TEXTURES_CONVERTED
    -f 2
)
_add_texture_convert_command(
    signage/indicator_lights/indicator_lights_corner_floor
    signage/indicator_lights/indicator_lights_corner_floor_on
    TEXTURES_CONVERTED
    -f 2
)
_add_texture_convert_command(
    signage/indicator_lights/indicator_lights_floor
    signage/indicator_lights/indicator_lights_floor_on
    TEXTURES_CONVERTED
    -f 2
)

foreach(TEXTURE_SCRIPT ${TEXTURE_SCRIPTS})
    cmake_path(
        REMOVE_EXTENSION TEXTURE_SCRIPT
        OUTPUT_VARIABLE TEXTURE_NAME
    )

    set(DEPENDENCY_TEXTURES ${TEXTURE_NAME})
    get_source_file_property(DEP_LIST ${TEXTURE_SCRIPT} ADDITIONAL_DEPS)
    if (DEP_LIST)
        list(APPEND DEPENDENCY_TEXTURES ${DEP_LIST})
    endif()

    # Define default conversion for textures not already handled
    foreach(TEXTURE ${DEPENDENCY_TEXTURES})
        _texture_has_convert_command(${TEXTURE} HAS_COMMAND)
        if (NOT HAS_COMMAND)
            _add_texture_convert_command(${TEXTURE} ${TEXTURE} TEXTURES_CONVERTED)
        endif()
    endforeach()
endforeach()

add_custom_target(textures_untransformed
    DEPENDS ${TEXTURES_CONVERTED}
)

############################
## Texture transformation ##
############################

function(_add_texture_transform_command TEXTURE_SCRIPT OUTPUT_LIST)
    cmake_path(
        REMOVE_EXTENSION TEXTURE_SCRIPT
        OUTPUT_VARIABLE TEXTURE_NAME
    )

    set(TEXTURE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURE_SCRIPT}")
    set(INPUT_FILE "${PAK_MATERIALS_DIR}/${TEXTURE_NAME}.png")
    set(OUTPUT_FILE "${PAK_MODIFIED_MATERIALS_DIR}/${TEXTURE_NAME}.png")

    set(DEPENDENCY_FILES ${INPUT_FILE})
    _get_property_image_files(${TEXTURE_SCRIPT} ADDITIONAL_DEPS ${PAK_MATERIALS_DIR} DEPENDENCY_FILES)

    set(OUTPUT_FILES ${OUTPUT_FILE})
    _get_property_image_files(${TEXTURE_SCRIPT} ADDITIONAL_OUTPUTS ${PAK_MODIFIED_MATERIALS_DIR} OUTPUT_FILES)

    add_custom_command(
        DEPENDS
            textures_untransformed ${DEPENDENCY_FILES}
            ${RUN_COMMAND} ${TEXTURE_SCRIPT}
        OUTPUT
            ${OUTPUT_FILES}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>"
        COMMAND
            ${Python3_EXECUTABLE} ${RUN_COMMAND}
            ${ImageMagickConvert_EXECUTABLE}
            ${INPUT_FILE}
            @${TEXTURE_SCRIPT}
            ${OUTPUT_FILE}
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILES})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for transforming converted PNG textures (crop, scale, etc.)

set(TEXTURES_TRANSFORMED "")

foreach(TEXTURE_SCRIPT ${TEXTURE_SCRIPTS})
    _add_texture_transform_command(${TEXTURE_SCRIPT} TEXTURES_TRANSFORMED)
endforeach()

add_custom_target(textures
    DEPENDS ${TEXTURES_TRANSFORMED}
)
set_target_properties(textures PROPERTIES
    # Enable dependents to easily depend on transformed textures
    OUTPUTS "${TEXTURES_TRANSFORMED}"
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
    set(OUTPUT_FILE_NO_EXTENSION "${GENERATED_CODE_DIR}/${RELATIVE_DIR}/${MATERIAL_NAME}")
    set(OUTPUT_FILE_H "${OUTPUT_FILE_NO_EXTENSION}.h")
    set(OUTPUT_FILE_C "${OUTPUT_FILE_NO_EXTENSION}_mat.c")

    # Depend on all textures for simplicity
    set(DEPENDENCY_FILES ${INPUT_FILE} ${TEXTURES_TRANSFORMED})
    _get_property_image_files(${INPUT_FILE} ADDITIONAL_DEPS ${ASSETS_DIR} DEPENDENCY_FILES)

    add_custom_command(
        DEPENDS
            textures ${DEPENDENCY_FILES}
            skeletool64::skeletool64
        OUTPUT
            ${OUTPUT_FILE_H} ${OUTPUT_FILE_C}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE_NO_EXTENSION}>"
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
# There are some special cases for materials which use textures not in the original game

set_property(SOURCE static.skm.yaml
    PROPERTY ADDITIONAL_DEPS
        images/turret
)

set_property(SOURCE hud.skm.yaml
    PROPERTY ADDITIONAL_DEPS
        images/center_reticle
)

set_property(SOURCE ui.skm.yaml
    PROPERTY ADDITIONAL_DEPS
        fonts/dejavu_sans/dejavu_sans_0
        fonts/dejavu_sans/dejavu_sans_1
        fonts/dejavu_sans/dejavu_sans_2
        fonts/dejavu_sans/dejavu_sans_3
        fonts/dejavu_sans/dejavu_sans_4
        fonts/liberation_mono/liberation_mono_0
        images/button_icons
        images/credits_icons
        images/github_qr
        images/rounded_corners
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
