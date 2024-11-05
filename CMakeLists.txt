cmake_minimum_required(VERSION 3.28)
project(portal64 C ASM)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(AddExternalProjectExecutable)

# Programs used in build
find_package(Blender                3.6 EXACT   REQUIRED)
find_program(FFmpeg_EXECUTABLE      ffmpeg      REQUIRED)
find_program(Git_EXECUTABLE         git         REQUIRED)
find_package(ImageMagickConvert     6 EXACT     REQUIRED)
find_program(NodeJs_EXECUTABLE      node        REQUIRED)
find_package(Python3                            REQUIRED)
find_program(SFZ2N64_EXECUTABLE     sfz2n64     REQUIRED)
find_program(Sox_EXECUTABLE         sox         REQUIRED)
find_program(VPK_EXECUTABLE         vpk         REQUIRED)
find_program(VTF2PNG_EXECUTABLE     vtf2png     REQUIRED)

# Add skeletool as external project to isolate from toolchain
add_external_project_executable(skeletool64)

# Game file directories
set(ASM_DIR "${PROJECT_SOURCE_DIR}/asm")
set(ASSETS_DIR "${PROJECT_SOURCE_DIR}/assets")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(VPK_DIR "${PROJECT_SOURCE_DIR}/vpk")

# Asset extraction/conversion output directories
set(PAK_DIR "${PROJECT_SOURCE_DIR}/portal_pak_dir")
set(PAK_MODIFIED_DIR "${PROJECT_SOURCE_DIR}/portal_pak_modified")

add_library(platform INTERFACE)
find_package(Libultra REQUIRED)
target_link_libraries(platform INTERFACE libultra::libultra)
target_compile_definitions(platform INTERFACE F3DEX_GBI_2)
target_include_directories(platform INTERFACE
    "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(${ASM_DIR})
add_subdirectory(${ASSETS_DIR})
add_subdirectory(${SRC_DIR})
add_subdirectory(${VPK_DIR})

add_library(code_segment STATIC)
target_link_libraries(code_segment PRIVATE
    platform

    engine
    level_list
    materials_core
    model_lists
    models_static
    sound_lookup_tables
    string_lookup_tables
)

add_executable(portal)
target_link_libraries(portal PRIVATE
    asm_sources
    code_segment
    levels
    models_dynamic
    string_data_tables
)

if (N64)
    # Generate linker script
    set(INPUT_LINKER_SCRIPT  "${PROJECT_SOURCE_DIR}/portal.ld")
    set(OUTPUT_LINKER_SCRIPT "${LINKER_SCRIPT_DIR}/portal.ld")
    add_custom_command(
        DEPENDS
            ${INPUT_LINKER_SCRIPT}
            anims_linker_script
            level_linker_script
            models_dynamic_linker_script
            string_linker_script
        OUTPUT
            ${OUTPUT_LINKER_SCRIPT}
        COMMAND
            ${CMAKE_CPP} -P -Wall -Werror
            -I${LINKER_SCRIPT_DIR}
            -o ${OUTPUT_LINKER_SCRIPT} ${INPUT_LINKER_SCRIPT}
        COMMENT
            "Generating $<PATH:RELATIVE_PATH,${OUTPUT_LINKER_SCRIPT},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
    add_custom_target(linker_script
        DEPENDS ${OUTPUT_LINKER_SCRIPT}
    )
    add_dependencies(portal linker_script)

    # Link
    set(LINKER_MAP_FILE "${LINKER_SCRIPT_DIR}/portal.map")
    target_link_options(portal PRIVATE
        -T ${OUTPUT_LINKER_SCRIPT}
        -Map ${LINKER_MAP_FILE}
    )
endif()
