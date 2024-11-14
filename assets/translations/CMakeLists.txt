set(BUILD_AUDIO_DIR "${PROJECT_BINARY_DIR}/src/audio")

set(GENERATE_STRINGS "${PROJECT_SOURCE_DIR}/tools/text/subtitle_generate.py")

##################
## Translations ##
##################

set(STRING_INPUT_FILES "")

set(SUBTITLES_H "${BUILD_AUDIO_DIR}/subtitles.h")
set(SUBTITLES_C "${BUILD_AUDIO_DIR}/subtitles.c")
set(STRING_LOOKUP_TABLE_FILES
    ${SUBTITLES_H}
    ${SUBTITLES_C}
)
set(STRING_DATA_TABLE_FILES "")

foreach(LANGUAGE ${TEXT_LANGUAGE_LIST})
    list(APPEND STRING_INPUT_FILES
        "${VPK_DIR}/Portal/hl2/resource/gameui_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/hl2/resource/valve_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/portal/resource/closecaption_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/portal/resource/portal_${LANGUAGE}.txt"
        "extra_${LANGUAGE}.txt"
    )

    list(APPEND STRING_DATA_TABLE_FILES
        "${BUILD_AUDIO_DIR}/subtitles_${LANGUAGE}.c"
    )
endforeach()

add_custom_command(
    DEPENDS
        ${STRING_DEPENDENCIES} ${GENERATE_STRINGS}
    OUTPUT
        ${STRING_LOOKUP_TABLE_FILES}
        ${STRING_DATA_TABLE_FILES}
    COMMAND
        ${Python3_EXECUTABLE} ${GENERATE_STRINGS}
        --languages ${TEXT_LANGUAGES}
        --game-root-dir ${VPK_DIR}
        --extra-translations-dir ${CMAKE_CURRENT_SOURCE_DIR}
        --output-dir ${BUILD_AUDIO_DIR}
    COMMENT
        "Generating strings"
    VERBATIM
)

add_custom_target(strings
    DEPENDS
        ${STRING_LOOKUP_TABLE_FILES}
        ${STRING_DATA_TABLE_FILES}
)

add_library(string_data_tables OBJECT)
add_dependencies(string_data_tables strings)
target_sources(string_data_tables PRIVATE
    ${STRING_DATA_TABLE_FILES}
)

add_library(string_lookup_tables INTERFACE)
add_dependencies(string_lookup_tables strings)
target_sources(string_lookup_tables INTERFACE
    ${STRING_LOOKUP_TABLE_FILES}
)

# Add command for generating strings linker script fragment

set(STRING_LINKER_SCRIPT "${LINKER_SCRIPT_DIR}/subtitles.ld")
add_custom_command(
    DEPENDS
        ${GEN_SEGMENT_LD}
    OUTPUT
        ${STRING_LINKER_SCRIPT}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_SEGMENT_LD}
        --alignment 16
        ${STRING_LINKER_SCRIPT}
        0x04000000
        "$<TARGET_OBJECTS:string_data_tables>"
    COMMENT
        "Generating $<PATH:RELATIVE_PATH,${STRING_LINKER_SCRIPT},${PROJECT_SOURCE_DIR}>"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(string_linker_script
    DEPENDS ${STRING_LINKER_SCRIPT}
)
