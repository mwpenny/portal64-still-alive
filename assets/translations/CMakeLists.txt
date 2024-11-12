# TODO: multiple languages

set(SUBTITLE_GENERATE "${PROJECT_SOURCE_DIR}/tools/subtitle_generate.py")

##################
## Translations ##
##################

set(STRINGS
    "${VPK_DIR}/Portal/hl2/resource/gameui_english.txt"
    "${VPK_DIR}/Portal/hl2/resource/valve_english.txt"
    "${VPK_DIR}/Portal/portal/resource/closecaption_english.txt"
    "${VPK_DIR}/Portal/portal/resource/portal_english.txt"
    extra_english.txt
)

set(STRING_DEPENDENCIES
    ${STRINGS}
    "${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_all.json"
)

# TODO: rename/relocate somewhere else. "Subtitles" also contain UI strings.
set(BUILD_AUDIO_DIR "${PROJECT_BINARY_DIR}/src/audio")
set(SUBTITLES_H "${BUILD_AUDIO_DIR}/subtitles.h")
set(SUBTITLES_C "${BUILD_AUDIO_DIR}/subtitles.c")
set(STRING_LOOKUP_TABLE_FILES
    ${SUBTITLES_H}
    ${SUBTITLES_C}
)
set(STRING_DATA_TABLE_FILES
    # TODO: make this dynamic
    ${BUILD_AUDIO_DIR}/subtitles_brazilian.c
    ${BUILD_AUDIO_DIR}/subtitles_bulgarian.c
    ${BUILD_AUDIO_DIR}/subtitles_czech.c
    ${BUILD_AUDIO_DIR}/subtitles_danish.c
    ${BUILD_AUDIO_DIR}/subtitles_dutch.c
    ${BUILD_AUDIO_DIR}/subtitles_english.c
    ${BUILD_AUDIO_DIR}/subtitles_finnish.c
    ${BUILD_AUDIO_DIR}/subtitles_french.c
    ${BUILD_AUDIO_DIR}/subtitles_german.c
    ${BUILD_AUDIO_DIR}/subtitles_greek.c
    ${BUILD_AUDIO_DIR}/subtitles_hungarian.c
    ${BUILD_AUDIO_DIR}/subtitles_italian.c
    ${BUILD_AUDIO_DIR}/subtitles_latam.c
    ${BUILD_AUDIO_DIR}/subtitles_norwegian.c
    ${BUILD_AUDIO_DIR}/subtitles_polish.c
    ${BUILD_AUDIO_DIR}/subtitles_portuguese.c
    ${BUILD_AUDIO_DIR}/subtitles_romanian.c
    ${BUILD_AUDIO_DIR}/subtitles_russian.c
    ${BUILD_AUDIO_DIR}/subtitles_spanish.c
    ${BUILD_AUDIO_DIR}/subtitles_swedish.c
    ${BUILD_AUDIO_DIR}/subtitles_turkish.c
    ${BUILD_AUDIO_DIR}/subtitles_ukrainian.c
)
add_custom_command(
    DEPENDS
        ${STRING_DEPENDENCIES} ${SUBTITLE_GENERATE}
    OUTPUT
        ${STRING_LOOKUP_TABLE_FILES}
        ${STRING_DATA_TABLE_FILES}
    COMMAND
        # TODO: update script to have configurable output location
        ${Python3_EXECUTABLE} ${SUBTITLE_GENERATE}
    WORKING_DIRECTORY
        # TODO: Update once script is refactored
        ${PROJECT_SOURCE_DIR}
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
