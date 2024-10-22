cmake_minimum_required(VERSION 3.28)
project(portal64 C ASM)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# TODO: remove when makefile is no longer in use
add_compile_definitions(CMAKE)

set(USING_LIBULTRA TRUE)

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
set(SKELETOOL_BUILD_DIR "${CMAKE_BINARY_DIR}/skeletool64")
ExternalProject_Add(skeletool64
    SOURCE_DIR
        "${PROJECT_SOURCE_DIR}/skelatool64"
    PREFIX
        "${CMAKE_BINARY_DIR}/skeletool64"
    CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
    BUILD_BYPRODUCTS
        "<INSTALL_DIR>/bin/skeletool64"
    BUILD_ALWAYS TRUE
)
add_executable(skeletool64::skeletool64 IMPORTED)
set_target_properties(skeletool64::skeletool64 PROPERTIES
    IMPORTED_LOCATION "${SKELETOOL_BUILD_DIR}/bin/skeletool64"
)
add_dependencies(skeletool64::skeletool64 skeletool64)

# Directories
set(ASM_DIR "${PROJECT_SOURCE_DIR}/asm")
set(ASSETS_DIR "${PROJECT_SOURCE_DIR}/assets")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(VPK_DIR "${PROJECT_SOURCE_DIR}/vpk")

set(PAK_DIR "${PROJECT_SOURCE_DIR}/portal_pak_dir")
set(PAK_MODIFIED_DIR "${PROJECT_SOURCE_DIR}/portal_pak_modified")

add_subdirectory(${ASM_DIR})
add_subdirectory(${ASSETS_DIR})
add_subdirectory(${SRC_DIR})
add_subdirectory(${VPK_DIR})
