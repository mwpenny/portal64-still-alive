cmake_minimum_required(VERSION 3.28)
project(portal64 C ASM)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(AddExternalProjectExecutable)
include(ListOption)

list_option(AUDIO_LANGUAGES "english"
    "Game audio languages to include"
    "${SUPPORTED_AUDIO_LANGUAGES}"
)
list_option(TEXT_LANGUAGES "all"
    "Text languages to include"
    "${SUPPORTED_TEXT_LANGUAGES}"
)

# Programs used in build
find_package(Blender                3.6 EXACT        REQUIRED)
find_program(FFmpeg_EXECUTABLE      ffmpeg           REQUIRED)
find_program(Git_EXECUTABLE         git              REQUIRED)
find_package(ImageMagickConvert     6.9.12.85...<7   REQUIRED)
find_package(NodeJs                 18.3.0           REQUIRED)
find_package(Python3                                 REQUIRED)
find_program(SFZ2N64_EXECUTABLE     sfz2n64          REQUIRED)
find_program(Sox_EXECUTABLE         sox              REQUIRED)
find_program(VPK_EXECUTABLE         vpk              REQUIRED)
find_program(VTF2PNG_EXECUTABLE     vtf2png          REQUIRED)

# Add skeletool as external project to isolate from toolchain
add_external_project_executable(skeletool64)

# Source and asset file directories
set(ASM_DIR            "${PROJECT_SOURCE_DIR}/asm")
set(ASSETS_DIR         "${PROJECT_SOURCE_DIR}/assets")
set(LINKER_DIR         "${PROJECT_SOURCE_DIR}/linker")
set(SRC_DIR            "${PROJECT_SOURCE_DIR}/src")
set(VPK_DIR            "${PROJECT_SOURCE_DIR}/vpk")

# Extracted/generated file output directories
set(GENERATED_CODE_DIR "${PROJECT_BINARY_DIR}/codegen")
set(LINKER_SCRIPT_DIR  "${PROJECT_BINARY_DIR}/ld")
set(PAK_DIR            "${PROJECT_SOURCE_DIR}/portal_pak_dir")
set(PAK_MODIFIED_DIR   "${PROJECT_SOURCE_DIR}/portal_pak_modified")

add_library(platform INTERFACE)
find_package(Libultra REQUIRED)
target_link_libraries(platform INTERFACE libultra::libultra)
target_include_directories(platform INTERFACE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(${ASSETS_DIR})
add_subdirectory(${ASM_DIR})
add_subdirectory(${SRC_DIR})
add_subdirectory(${LINKER_DIR})
add_subdirectory(${VPK_DIR})

add_library(code_segment STATIC)
target_link_libraries(code_segment PRIVATE
    engine
    font_layout_tables
    level_list
    materials_core
    model_lists
    models_static
    sound_lookup_tables
    string_lookup_tables
)

add_executable(portal)
target_link_libraries(portal PRIVATE
    platform

    asm
    code_segment
    materials_images
    levels
    level_animations
    model_animations
    models_dynamic
    string_data_tables
)

if (N64)
    add_dependencies(portal linker_script)
    target_linker_script(portal "$<TARGET_PROPERTY:linker_script,SCRIPT_FILE>")

    add_n64_rom(portal)
endif()
