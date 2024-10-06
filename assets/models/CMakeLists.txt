cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
)

##################
## Model export ##
##################

# Models whose data is loaded/unloaded as needed
set(DYNAMIC_MODELS
    cube/cube
    props/autoportal_frame/autoportal_frame
    props/box_dropper_glass
    props/cylinder_test
    props/lab_chair
    props/lab_desk/lab_desk01
    props/lab_desk/lab_desk02
    props/lab_desk/lab_desk03
    props/lab_desk/lab_desk04
    props/lab_monitor
    props/light_rail_endcap
    props/portal_cleanser
    props/radio
    signage/clock
    signage/clock_digits
)

# Animated models whose data is loaded/unloaded as needed
set(DYNAMIC_ANIMATED_MODELS
    pedestal
    props/box_dropper
    props/button
    props/combine_ball_catcher
    props/combine_ball_launcher
    props/door_01
    props/door_02
    props/security_camera
    props/switch001
)

# All models
set(MODELS
    ${DYNAMIC_MODELS}
    ${DYNAMIC_ANIMATED_MODELS}
    fleck_ash2
    grav_flare
    player/chell
    portal_gun/ball_trail
    portal_gun/v_portalgun
    portal_gun/w_portalgun
    portal/portal_blue
    portal/portal_blue_face
    portal/portal_blue_filled
    portal/portal_collider
    portal/portal_collider_vertical
    portal/portal_orange
    portal/portal_orange_face
    portal/portal_orange_filled
    props/round_elevator
    props/round_elevator_collision
    props/round_elevator_interior
    props/signage
)

# Most models depend on both of these. Some only depend on one or the other,
# but use both for simplicity. Otherwise there would be many special cases.
set(MODEL_DEFAULT_MATERIALS
    ${ASSETS_DIR}/materials/static.skm.yaml
    ${ASSETS_DIR}/materials/objects.skm.yaml
)

set(GEN_DYNAMIC_MODEL_LIST "${PROJECT_SOURCE_DIR}/tools/models/generate_dynamic_model_list.js")
set(GEN_DYNAMIC_ANIM_MODEL_LIST "${PROJECT_SOURCE_DIR}/tools/models/generate_dynamic_animated_model_list.js")

function(_add_model_export_command MODEL_NAME OUTPUT_VARIABLE)
    set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}.blend")
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${MODEL_NAME}.fbx")

    cmake_path(
       GET OUTPUT_FILE PARENT_PATH
       OUTPUT_DIR
    )

    add_custom_command(
        DEPENDS
            ${EXPORT_FBX} ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
           ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${BLENDER_3_6} ${INPUT_FILE} --background --python ${EXPORT_FBX} -- ${OUTPUT_FILE}
        COMMENT
            "Exporting $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    set(${OUTPUT_VARIABLE} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_VARIABLE})
endfunction()

function(_add_model_generate_command MODEL_NAME MODEL_FBX OUTPUT_LIST)
    set(MODEL_FLAGS "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}.flags")
    set(MODEL_MATERIALS ${MODEL_DEFAULT_MATERIALS})

    get_source_file_property(FILE_MATERIALS ${MODEL_FLAGS} ADDITIONAL_MATERIALS)
    if (FILE_MATERIALS)
        list(APPEND MODEL_MATERIALS ${FILE_MATERIALS})
    endif()

    cmake_path(
        REMOVE_EXTENSION MODEL_FBX
        OUTPUT_VARIABLE OUTPUT_FILE_NO_EXTENSION
    )
    cmake_path(
       GET OUTPUT_FILE_NO_EXTENSION PARENT_PATH
       OUTPUT_DIR
    )

    set(OUTPUT_FILE_H "${OUTPUT_FILE_NO_EXTENSION}.h")
    set(OUTPUT_FILE_GEO_C "${OUTPUT_FILE_NO_EXTENSION}_geo.c")

    # TODO: not all models have animations
    # set(OUTPUT_FILE_ANIM_C "${OUTPUT_FILE_NO_EXTENSION}_anim.c")

    add_custom_command(
        DEPENDS
            ${CONVERT_ASSET} ${MODEL_FBX} ${MODEL_FLAGS} ${MODEL_MATERIALS}
        OUTPUT
            ${OUTPUT_FILE_H} ${OUTPUT_FILE_GEO_C} ${OUTPUT_FILE_ANIM_C}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${PYTHON3} ${CONVERT_ASSET}
            ${SKELETOOL64}
            ${MODEL_FBX}
            ${MODEL_FLAGS}
            --fixed-point-scale ${SCENE_SCALE}
            --model-scale ${MODEL_SCALE}
            --name ${MODEL_NAME}
            --output ${OUTPUT_FILE_H}
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Generating model ${MODEL_NAME}"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST}
        ${OUTPUT_FILE_H}
        ${OUTPUT_FILE_GEO_C}
        ${OUTPUT_FILE_ANIM_C}
    )
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()


# Add commands for exporting model files
# There are some special cases for models which use non-default materials

set_property(SOURCE player/chell.flags
    PROPERTY ADDITIONAL_MATERIALS
        ${ASSETS_DIR}/materials/chell.skm.yaml
)

set_property(SOURCE props/combine_ball_launcher.flags
    PROPERTY ADDITIONAL_MATERIALS
        ${ASSETS_DIR}/materials/ball_catcher.skm.yaml
)

set_property(SOURCE props/combine_ball_catcher.flags
    PROPERTY ADDITIONAL_MATERIALS
        ${ASSETS_DIR}/materials/ball_catcher.skm.yaml
)

set_property(SOURCE props/round_elevator.flags
    PROPERTY ADDITIONAL_MATERIALS
        ${ASSETS_DIR}/materials/elevator.skm.yaml
)

set_property(SOURCE props/round_elevator_interior.flags
    PROPERTY ADDITIONAL_MATERIALS
        ${ASSETS_DIR}/materials/elevator.skm.yaml
)

set(MODEL_GENERATED_FILES "")

foreach(MODEL ${MODELS})
    _add_model_export_command(${MODEL} MODEL_FBX)
    _add_model_generate_command(${MODEL} ${MODEL_FBX} MODEL_GENERATED_FILES)
endforeach()

add_custom_target(
    models
    DEPENDS ${MODEL_GENERATED_FILES}
)

# Add commands for generating dynamic model lists

set(MODEL_GENERATED_HEADERS ${MODEL_GENERATED_FILES})
list(FILTER MODEL_GENERATED_HEADERS INCLUDE REGEX "\.h$")

set(DYNAMIC_MODEL_HEADERS "")
set(DYNAMIC_ANIMATED_MODEL_HEADERS "")

foreach(GENERATED_HEADER ${MODEL_GENERATED_HEADERS})
    cmake_path(
        RELATIVE_PATH GENERATED_HEADER
        BASE_DIRECTORY "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}"
        OUTPUT_VARIABLE MODEL_NAME
    )
    cmake_path(
        REMOVE_EXTENSION MODEL_NAME
        OUTPUT_VARIABLE MODEL_NAME
    )

    if(MODEL_NAME IN_LIST DYNAMIC_MODELS)
        list(APPEND DYNAMIC_MODEL_HEADERS ${GENERATED_HEADER})
    elseif(MODEL_NAME IN_LIST DYNAMIC_ANIMATED_MODELS)
        list(APPEND DYNAMIC_ANIMATED_MODEL_HEADERS ${GENERATED_HEADER})
    endif()
endforeach()

set(DYNAMIC_MODEL_LIST_OUT_DIR "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}")

set(DYNAMIC_MODEL_LIST_H "${DYNAMIC_MODEL_LIST_OUT_DIR}/dynamic_model_list.h")
set(DYNAMIC_MODEL_LIST_C "${DYNAMIC_MODEL_LIST_OUT_DIR}/dynamic_model_list.c")
add_custom_command(
    DEPENDS
        ${GEN_DYNAMIC_MODEL_LIST} ${MODEL_LIST_UTILS} ${DYNAMIC_MODEL_HEADERS}
    OUTPUT
        ${DYNAMIC_MODEL_LIST_H} ${DYNAMIC_MODEL_LIST_C}
    COMMAND
        ${NODEJS} ${GEN_DYNAMIC_MODEL_LIST}
        ${DYNAMIC_MODEL_LIST_H}
        ${DYNAMIC_MODEL_HEADERS}
    COMMENT
        "Generating dynamic model list"
    VERBATIM
)

set(DYNAMIC_ANIMATED_MODEL_LIST_H "${DYNAMIC_MODEL_LIST_OUT_DIR}/dynamic_animated_model_list.h")
set(DYNAMIC_ANIMATED_MODEL_LIST_C "${DYNAMIC_MODEL_LIST_OUT_DIR}/dynamic_animated_model_list.c")
add_custom_command(
    DEPENDS
        ${GEN_DYNAMIC_ANIM_MODEL_LIST} ${MODEL_LIST_UTILS} ${DYNAMIC_ANIMATED_MODEL_HEADERS}
    OUTPUT
        ${DYNAMIC_ANIMATED_MODEL_LIST_H} ${DYNAMIC_ANIMATED_MODEL_LIST_C}
    COMMAND
        ${NODEJS} ${GEN_DYNAMIC_ANIM_MODEL_LIST}
        ${DYNAMIC_ANIMATED_MODEL_LIST_H}
        ${DYNAMIC_ANIMATED_MODEL_HEADERS}
    COMMENT
        "Generating dynamic animated model list"
    VERBATIM
)

add_custom_target(
    model_lists
    DEPENDS
        ${MODEL_GENERATED_FILES}
        ${DYNAMIC_MODEL_LIST_H}
        ${DYNAMIC_MODEL_LIST_C}
        ${DYNAMIC_ANIMATED_MODEL_LIST_H}
        ${DYNAMIC_ANIMATED_MODEL_LIST_C}
)
