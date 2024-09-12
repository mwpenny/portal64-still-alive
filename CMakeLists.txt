cmake_minimum_required(VERSION 3.28)
project(portal64)

# Tools
# TODO: find automatically
# TODO: test on Windows
set(FFMPEG "ffmpeg")
set(IMAGEMAGICK_CONVERT "convert")
set(MPG123 "mpg123")
set(NODEJS "node")
set(PYTHON3 "python")
set(SFZ2N64 "sfz2n64")
set(SOX "sox")
set(VPK "vpk")
set(VTF2PNG "vtf2png")

set(CONVERT_ASSET "${PROJECT_SOURCE_DIR}/tools/convert_asset.py")
set(GEN_SOUND_IDS "${PROJECT_SOURCE_DIR}/tools/generate_sound_ids.js")
set(JSOX "${PROJECT_SOURCE_DIR}/tools/jsox.js")
set(SKELETOOL64 "${PROJECT_SOURCE_DIR}/skelatool64/skeletool64")

# Directories
set(ASSETS_DIR "${PROJECT_SOURCE_DIR}/assets")
set(PAK_DIR "${PROJECT_SOURCE_DIR}/portal_pak_dir")
set(PAK_MODIFIED_DIR "${PROJECT_SOURCE_DIR}/portal_pak_modified")
set(VPK_DIR "${PROJECT_SOURCE_DIR}/vpk")

add_subdirectory(${ASSETS_DIR})
add_subdirectory(${VPK_DIR})
