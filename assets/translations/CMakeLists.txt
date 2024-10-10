# TODO: multiple languages

set(SUBTITLE_GENERATE "${PROJECT_SOURCE_DIR}/tools/subtitle_generate.py")

###############
## Subtitles ##
###############

set(STRINGS
    "${VPK_DIR}/Portal/hl2/resource/gameui_english.txt"
    "${VPK_DIR}/Portal/hl2/resource/valve_english.txt"
    "${VPK_DIR}/Portal/portal/resource/closecaption_english.txt"
    "${VPK_DIR}/Portal/portal/resource/portal_english.txt"
    extra_english.txt
)

set(STRING_DEPENDENCIES
    ${STRINGS}
    "${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans_book_8.json"
)

# TODO: rename/relocate somewhere else. "Subtitles" also contain UI strings.
set(BUILD_AUDIO_DIR "${CMAKE_BINARY_DIR}/src/audio")
set(SUBTITLES_H "${BUILD_AUDIO_DIR}/subtitles.h")
set(SUBTITLES_C "${BUILD_AUDIO_DIR}/subtitles.c")
set(STRING_GENERATED_FILES
    ${SUBTITLES_H}
    ${SUBTITLES_C}

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
        ${STRING_GENERATED_FILES}
    COMMAND
        # TODO: update script to have configurable output location
        ${PYTHON3} ${SUBTITLE_GENERATE}
    WORKING_DIRECTORY
        # TODO: Update once script is refactored
        ${PROJECT_SOURCE_DIR}
    COMMENT
        "Generating strings"
    VERBATIM
)

add_custom_target(generated_strings
    DEPENDS ${STRING_GENERATED_FILES}
)

add_library(strings INTERFACE)
add_dependencies(strings generated_strings)
target_sources(strings INTERFACE
    ${STRING_GENERATED_FILES}
)
