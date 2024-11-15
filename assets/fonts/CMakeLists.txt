set(FONT_CONVERTER "${PROJECT_SOURCE_DIR}/tools/text/font_converter.js")

###########
## Fonts ##
###########

set(DEJAVU_SANS_FILES
    dejavu_sans/dejavu_sans_0.json
    dejavu_sans/dejavu_sans_1.json
    dejavu_sans/dejavu_sans_2.json
    dejavu_sans/dejavu_sans_3.json
    dejavu_sans/dejavu_sans_4.json
    dejavu_sans/dejavu_sans_all.json
)

set(LIBERATION_MONO_FILES
    liberation_mono/liberation_mono_0.json
)

function(_add_convert_font_command FONT_NAME INPUT_FILES OUTPUT_LIST)
    cmake_path(
        RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
    )

    # Use font directory name for output file
    list(GET INPUT_FILES 0 FONT_DIR_NAME)
    cmake_path(
        GET FONT_DIR_NAME PARENT_PATH
        FONT_DIR_NAME
    )
    set(OUTPUT_FILE "${PROJECT_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${FONT_DIR_NAME}.c")

    add_custom_command(
        DEPENDS
            ${INPUT_FILES} ${FONT_CONVERTER}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${NodeJs_EXECUTABLE} ${FONT_CONVERTER}
            ${FONT_NAME} ${FONT_DIR_NAME} ${OUTPUT_FILE}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT
            "Generating $<PATH:RELATIVE_PATH,${OUTPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for generating font layout information

set(FONT_FILES "")

_add_convert_font_command(DejaVuSans     "${DEJAVU_SANS_FILES}"     FONT_FILES)
_add_convert_font_command(LiberationMono "${LIBERATION_MONO_FILES}" FONT_FILES)

add_custom_target(fonts
    DEPENDS ${FONT_FILES}
)

add_library(font_layout_tables INTERFACE)
add_dependencies(font_layout_tables fonts)
target_sources(font_layout_tables INTERFACE
    ${FONT_FILES}
)
