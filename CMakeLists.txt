cmake_minimum_required(VERSION 3.28)
project(portal64)

# Tools
# TODO: find automatically
# TODO: test on Windows
set(IMAGEMAGICK_CONVERT "convert")
set(VTF2PNG "vtf2png")
set(PYTHON3 "python")
set(VPK "vpk")
set(CONVERT_ASSET "${PROJECT_SOURCE_DIR}/tools/convert_asset.py")

# Directories
set(ASSETS_DIR "${PROJECT_SOURCE_DIR}/assets")
set(PAK_DIR "${PROJECT_SOURCE_DIR}/portal_pak_dir")
set(PAK_MODIFIED_DIR "${PROJECT_SOURCE_DIR}/portal_pak_modified")
set(VPK_DIR "${PROJECT_SOURCE_DIR}/vpk")

add_subdirectory(${ASSETS_DIR})
add_subdirectory(${VPK_DIR})
