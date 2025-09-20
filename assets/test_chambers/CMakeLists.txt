set(EXPORT_LEVEL ${PROJECT_SOURCE_DIR}/tools/level_scripts/export_level.lua)
set(EXPORT_SCRIPTS
    ${EXPORT_LEVEL}
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

set(GEN_LEVEL_LIST "${PROJECT_SOURCE_DIR}/tools/models/generate_level_list.js")

cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
)

##################
## Level export ##
##################

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
    test_chamber_11
)

set(ANIMATED_LEVELS
    test_chamber_00
    test_chamber_03
    test_chamber_04
    test_chamber_06
    test_chamber_07
    test_chamber_08
    test_chamber_09
    test_chamber_10
    test_chamber_11
)

set(LEVEL_MATERIALS
    ${ASSETS_DIR}/materials/static.skm.yaml
)

function(_add_level_export_command LEVEL_NAME OUTPUT_VARIABLE)
    set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.blend")
    set(OUTPUT_FILE "${PROJECT_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.fbx")

    add_custom_command(
        DEPENDS
            ${EXPORT_FBX} ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>"
        COMMAND
            ${Blender_EXECUTABLE}
            ${INPUT_FILE} --background --python ${EXPORT_FBX} -- ${OUTPUT_FILE}
        COMMENT
            "Exporting $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    set(${OUTPUT_VARIABLE} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_VARIABLE})
endfunction()

function(_add_level_generate_command LEVEL_NAME LEVEL_FBX LEVEL_OUTPUT_LIST ANIM_OUTPUT_LIST)
    set(LEVEL_YAML "${CMAKE_CURRENT_SOURCE_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.yaml")
    set(OUTPUT_FILE_NO_EXTENSION "${GENERATED_CODE_DIR}/${RELATIVE_CURRENT_DIR}/${LEVEL_NAME}/${LEVEL_NAME}")

    set(OUTPUT_FILE_H "${OUTPUT_FILE_NO_EXTENSION}.h")
    set(OUTPUT_FILE_GEO_C "${OUTPUT_FILE_NO_EXTENSION}_geo.c")
    set(LEVEL_OUTPUT_FILES
        ${OUTPUT_FILE_H}
        ${OUTPUT_FILE_GEO_C}
    )
    set(ANIM_OUTPUT_FILES "")

    if (LEVEL_NAME IN_LIST ANIMATED_LEVELS)
        set(OUTPUT_FILE_ANIM_C "${OUTPUT_FILE_NO_EXTENSION}_anim.c")
        list(APPEND ANIM_OUTPUT_FILES ${OUTPUT_FILE_ANIM_C})
    endif()

    set(MATERIAL_ARGS "")
    foreach(MATERIAL ${LEVEL_MATERIALS})
        list(APPEND MATERIAL_ARGS
            -m ${MATERIAL}
        )
    endforeach()

    add_custom_command(
        DEPENDS
            textures "$<TARGET_PROPERTY:textures,OUTPUTS>"
            skeletool64::skeletool64
            ${EXPORT_SCRIPTS}
            ${LEVEL_FBX}
            ${LEVEL_YAML}
            ${LEVEL_MATERIALS}
        OUTPUT
            ${LEVEL_OUTPUT_FILES}
            ${ANIM_OUTPUT_FILES}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE_NO_EXTENSION}>"
        COMMAND
            skeletool64::skeletool64
            --script ${EXPORT_LEVEL}
            --fixed-point-scale ${SCENE_SCALE}
            --model-scale ${MODEL_SCALE}
            --name ${LEVEL_NAME}/${LEVEL_NAME}
            ${MATERIAL_ARGS}
            --output ${OUTPUT_FILE_H}
            ${LEVEL_FBX}
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Generating level ${LEVEL_NAME}"
        VERBATIM
    )

    list(APPEND ${LEVEL_OUTPUT_LIST} ${LEVEL_OUTPUT_FILES})
    list(APPEND ${ANIM_OUTPUT_LIST} ${ANIM_OUTPUT_FILES})
    return(PROPAGATE ${LEVEL_OUTPUT_LIST} ${ANIM_OUTPUT_LIST})
endfunction()

function(_add_level_lib LIB_NAME LIB_SOURCES)
    add_library(${LIB_NAME} OBJECT)
    add_dependencies(${LIB_NAME}
        materials
        sound_lookup_tables
        string_lookup_tables
        generated_levels
    )
    target_sources(${LIB_NAME} PRIVATE
        ${LIB_SOURCES}
    )
    target_link_libraries(${LIB_NAME}
        platform
    )
endfunction()

# Add commands for exporting level files

set(LEVEL_GENERATED_FILES "")
set(ANIM_GENERATED_FILES "")

foreach(LEVEL ${LEVELS})
    _add_level_export_command(${LEVEL} LEVEL_FBX)
    _add_level_generate_command(${LEVEL} ${LEVEL_FBX}
        LEVEL_GENERATED_FILES
        ANIM_GENERATED_FILES
    )
endforeach()

add_custom_target(generated_levels
    DEPENDS
        ${LEVEL_GENERATED_FILES}
        ${ANIM_GENERATED_FILES}
)

_add_level_lib(levels           "${LEVEL_GENERATED_FILES}")
_add_level_lib(level_animations "${ANIM_GENERATED_FILES}")

# Add command for generating level list header

set(LEVEL_GENERATED_HEADERS ${LEVEL_GENERATED_FILES})
list(FILTER
    LEVEL_GENERATED_HEADERS
    INCLUDE REGEX "\.h$"
)

set(LEVEL_LIST_OUT_DIR "${GENERATED_CODE_DIR}/${RELATIVE_CURRENT_DIR}")
set(LEVEL_LIST "${LEVEL_LIST_OUT_DIR}/level_list.h")

add_custom_command(
    DEPENDS
        generated_levels ${LEVEL_GENERATED_HEADERS}
        ${GEN_LEVEL_LIST} ${MODEL_LIST_UTILS}
    OUTPUT
        ${LEVEL_LIST}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_LEVEL_LIST}
        ${LEVEL_LIST} ${LEVEL_GENERATED_HEADERS}
    WORKING_DIRECTORY
        ${LEVEL_LIST_OUT_DIR}
    COMMENT
        "Generating $<PATH:RELATIVE_PATH,${LEVEL_LIST},${PROJECT_SOURCE_DIR}>"
    VERBATIM
)

add_custom_target(generated_level_list
    DEPENDS ${LEVEL_LIST}
)

add_library(level_list INTERFACE)
add_dependencies(level_list generated_level_list)
target_sources(level_list INTERFACE
    ${LEVEL_LIST}
)

# Add command for generating level linker script fragment

set(LEVEL_LINKER_SCRIPT "${LINKER_SCRIPT_DIR}/levels.ld")
add_custom_command(
    DEPENDS
        ${GEN_SEGMENT_LD}
    OUTPUT
        ${LEVEL_LINKER_SCRIPT}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_SEGMENT_LD}
        ${LEVEL_LINKER_SCRIPT}
        0x02000000
        "$<TARGET_OBJECTS:levels>"
    COMMENT
        "Generating $<PATH:RELATIVE_PATH,${LEVEL_LINKER_SCRIPT},${PROJECT_SOURCE_DIR}>"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(level_linker_script
    DEPENDS ${LEVEL_LINKER_SCRIPT}
)
set_target_properties(level_linker_script PROPERTIES
    SCRIPT_FILE "${LEVEL_LINKER_SCRIPT}"
)
