set(BUILD_STRINGS_DIR "${GENERATED_CODE_DIR}/assets/strings")

set(FONT_DEF          "${PROJECT_SOURCE_DIR}/assets/fonts/dejavu_sans/dejavu_sans_all.json")
set(GENERATE_STRINGS  "${PROJECT_SOURCE_DIR}/tools/text/generate_strings.py")

##################
## Translations ##
##################

set(STRING_INPUT_FILES "")

set(STRINGS_H "${BUILD_STRINGS_DIR}/strings.h")
set(STRINGS_C "${BUILD_STRINGS_DIR}/strings.c")
set(STRING_LOOKUP_TABLE_FILES
    ${STRINGS_H}
    ${STRINGS_C}
)
set(STRING_DATA_TABLE_FILES "")

foreach(LANGUAGE ${TEXT_LANGUAGES_LIST})
    list(APPEND STRING_INPUT_FILES
        "${VPK_DIR}/Portal/hl2/resource/closecaption_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/hl2/resource/gameui_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/hl2/resource/valve_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/portal/resource/closecaption_${LANGUAGE}.txt"
        "${VPK_DIR}/Portal/portal/resource/portal_${LANGUAGE}.txt"
        "extra_${LANGUAGE}.txt"
    )

    list(APPEND STRING_DATA_TABLE_FILES
        "${BUILD_STRINGS_DIR}/strings_${LANGUAGE}.c"
    )
endforeach()

add_custom_command(
    DEPENDS
        ${STRING_INPUT_FILES} ${FONT_DEF} ${GENERATE_STRINGS}
    OUTPUT
        ${STRING_LOOKUP_TABLE_FILES}
        ${STRING_DATA_TABLE_FILES}
    COMMAND
        ${Python3_EXECUTABLE} ${GENERATE_STRINGS}
        --languages ${TEXT_LANGUAGES}
        --game-root-dir ${VPK_DIR}
        --extra-translations-dir ${CMAKE_CURRENT_SOURCE_DIR}
        --output-dir ${BUILD_STRINGS_DIR}
        --check-font ${FONT_DEF}
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

set(STRING_LINKER_SCRIPT "${LINKER_SCRIPT_DIR}/strings.ld")
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
set_target_properties(string_linker_script PROPERTIES
    SCRIPT_FILE "${STRING_LINKER_SCRIPT}"
)
