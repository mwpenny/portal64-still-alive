cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
)

##################
## Level export ##
##################

set(SCENE_SCALE 128)
set(MODEL_SCALE 0.01)

set(LEVELS
    test_chamber_00
    test_chamber_01
    test_chamber_02
    test_chamber_03
    test_chamber_04
    test_chamber_05
    test_chamber_06
    test_chamber_07
    test_chamber_08
    test_chamber_09
    test_chamber_10
)

set(LEVEL_MATERIALS
    ${ASSETS_DIR}/materials/static.skm.yaml
)

set(EXPORT_LEVEL ${PROJECT_SOURCE_DIR}/tools/level_scripts/export_level.lua)
set(EXPORT_SCRIPTS
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/animation.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/collision_export.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/dynamic_collision_export.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/entities.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/portal_surfaces.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/room_export.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/signals.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/static_export.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/trigger.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/util.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/world.lua
    ${PROJECT_SOURCE_DIR}/tools/level_scripts/yaml_loader.lua
)

function(_add_level_export_command LEVEL_NAME OUTPUT_VARIABLE)
    set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.blend")
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.fbx")

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

function(_add_level_generate_command LEVEL_NAME LEVEL_FBX OUTPUT_LIST)
    set(LEVEL_YAML "${CMAKE_CURRENT_SOURCE_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.yaml")

    cmake_path(
        REMOVE_EXTENSION LEVEL_FBX
        OUTPUT_VARIABLE OUTPUT_FILE_NO_EXTENSION
    )

    set(OUTPUT_FILE_H "${OUTPUT_FILE_NO_EXTENSION}.h")
    set(OUTPUT_FILE_GEO_C "${OUTPUT_FILE_NO_EXTENSION}_geo.c")

    # TODO: not all levels have animations
    # set(OUTPUT_FILE_ANIM_C "${OUTPUT_FILE_NO_EXTENSION}_anim.c")

    set(MATERIAL_ARGS "")
    foreach(MATERIAL ${LEVEL_MATERIALS})
        list(APPEND MATERIAL_ARGS
            -m ${MATERIAL}
        )
    endforeach()

    add_custom_command(
        DEPENDS
            ${EXPORT_LEVEL} ${LEVEL_FBX} ${LEVEL_YAML} ${LEVEL_MATERIALS}
        OUTPUT
            ${OUTPUT_FILE_H} ${OUTPUT_FILE_GEO_C} ${OUTPUT_FILE_ANIM_C}
        COMMAND
            ${SKELETOOL64}
            --script ${EXPORT_LEVEL}
            --fixed-point-scale ${SCENE_SCALE}
            --model-scale ${MODEL_SCALE}
            --name ${LEVEL_NAME}
            ${MATERIAL_ARGS}
            --output ${OUTPUT_FILE_H}
            ${LEVEL_FBX}
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Generating level ${LEVEL_NAME}"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST}
        ${OUTPUT_FILE_H}
        ${OUTPUT_FILE_GEO_C}
        ${OUTPUT_FILE_ANIM_C}
    )
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for exporting level files

set(LEVEL_GENERATED_FILES "")

foreach(LEVEL ${LEVELS})
    _add_level_export_command(${LEVEL} LEVEL_FBX)
    _add_level_generate_command(${LEVEL} ${LEVEL_FBX} LEVEL_GENERATED_FILES)
endforeach()

add_custom_target(
    all_levels
    DEPENDS ${LEVEL_GENERATED_FILES}
)

# Add command for generating level list header

set(LEVEL_GENERATED_HEADERS ${LEVEL_GENERATED_FILES})
list(FILTER LEVEL_GENERATED_HEADERS INCLUDE REGEX "\.h$")

set(LEVEL_LIST_OUT_DIR "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}")
set(LEVEL_LIST "${LEVEL_LIST_OUT_DIR}/level_list.h")

add_custom_command(
    DEPENDS
        ${GENERATE_LEVEL_LIST} ${LEVEL_GENERATED_HEADERS}
    OUTPUT
        ${LEVEL_LIST}
    COMMAND
        ${NODEJS} ${GEN_LEVEL_LIST} ${LEVEL_LIST} ${LEVEL_GENERATED_HEADERS}
    WORKING_DIRECTORY
        ${LEVEL_LIST_OUT_DIR}
    COMMENT
        "Generating $<PATH:RELATIVE_PATH,${LEVEL_LIST},${PROJECT_SOURCE_DIR}>"
    VERBATIM
)

# TODO: remove
add_custom_target(
    level_list
    DEPENDS ${LEVEL_LIST}
)
