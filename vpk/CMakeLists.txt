set(NORMALIZE_SOUND_PATHS  "${PROJECT_SOURCE_DIR}/tools/sound/normalize_sound_paths.js")

####################
## VPK extraction ##
####################

# TODO: automatic discovery of game files
set(VPKS_PORTAL
    Portal/portal/portal_pak_dir.vpk
    Portal/portal/portal_pak_000.vpk
    Portal/portal/portal_pak_001.vpk
    Portal/portal/portal_pak_002.vpk
    Portal/portal/portal_pak_003.vpk
    Portal/portal/portal_pak_004.vpk
    Portal/portal/portal_pak_005.vpk
)

set(VPKS_HL2_MISC
    Portal/hl2/hl2_misc_dir.vpk
    Portal/hl2/hl2_misc_000.vpk
    Portal/hl2/hl2_misc_001.vpk
    Portal/hl2/hl2_misc_002.vpk
    Portal/hl2/hl2_misc_003.vpk
)

set(VPKS_HL2_SOUND_MISC
    Portal/hl2/hl2_sound_misc_dir.vpk
    Portal/hl2/hl2_sound_misc_000.vpk
    Portal/hl2/hl2_sound_misc_001.vpk
    Portal/hl2/hl2_sound_misc_002.vpk
)

set(VPKS_HL2_TEXTURES
    Portal/hl2/hl2_textures_dir.vpk
    Portal/hl2/hl2_textures_000.vpk
    Portal/hl2/hl2_textures_001.vpk
    Portal/hl2/hl2_textures_002.vpk
    Portal/hl2/hl2_textures_003.vpk
    Portal/hl2/hl2_textures_004.vpk
    Portal/hl2/hl2_textures_005.vpk
    Portal/hl2/hl2_textures_006.vpk
    Portal/hl2/hl2_textures_007.vpk
    Portal/hl2/hl2_textures_008.vpk
    Portal/hl2/hl2_textures_009.vpk
    Portal/hl2/hl2_textures_010.vpk
    Portal/hl2/hl2_textures_011.vpk
)

function(_add_vpk_extract_command VPK_LIST OUTPUT_DIR OUTPUT_LIST)
    # If the files do not exist at configuration time, CMake will treat the
    # paths as relative to the build directory, even once they are present.
    # Use absolute paths to avoid this problem for users who forget to copy
    # their game files before running CMake.
    list(TRANSFORM VPK_LIST PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/" OUTPUT_VARIABLE VPK_LIST)

    # First VPK is the one to extract
    list(GET VPK_LIST 0 MAIN_VPK)

    cmake_path(
        GET MAIN_VPK STEM
        MAIN_VPK_NAME
    )
    set(MAIN_VPK_STAMP "${PAK_DIR}/${MAIN_VPK_NAME}.stamp")

    add_custom_command(
        DEPENDS
            ${VPK_LIST}
        OUTPUT
            ${MAIN_VPK_STAMP}
        COMMAND
            ${VPK_EXECUTABLE} -x ${OUTPUT_DIR} ${ARGN} ${MAIN_VPK}
        COMMAND
            ${CMAKE_COMMAND} -E touch ${MAIN_VPK_STAMP}
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT
            "Extracting ${MAIN_VPK}"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${MAIN_VPK_STAMP})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

function(_add_normalize_localized_audio_command STAMP_FILE_LIST)
    set(AUDIO_FIX_STAMP "${PAK_DIR}/sound_path_normalization.stamp")

    add_custom_command(
        DEPENDS
            ${NORMALIZE_SOUND_PATHS}
            ${${STAMP_FILE_LIST}}
        OUTPUT
            ${AUDIO_FIX_STAMP}
        COMMAND
            ${NodeJs_EXECUTABLE}
            ${NORMALIZE_SOUND_PATHS} ${PAK_DIR}
        COMMAND
            ${CMAKE_COMMAND} -E touch ${AUDIO_FIX_STAMP}
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT
            "Normalizing localized audio file names"
        VERBATIM
    )

    list(APPEND ${STAMP_FILE_LIST} ${AUDIO_FIX_STAMP})
    return(PROPAGATE ${STAMP_FILE_LIST})
endfunction()

set(VPK_STAMP_FILES "")

_add_vpk_extract_command("${VPKS_PORTAL}"         ${PAK_DIR} VPK_STAMP_FILES)
_add_vpk_extract_command("${VPKS_HL2_MISC}"       ${PAK_DIR} VPK_STAMP_FILES)
_add_vpk_extract_command("${VPKS_HL2_SOUND_MISC}" ${PAK_DIR} VPK_STAMP_FILES)
_add_vpk_extract_command("${VPKS_HL2_TEXTURES}"   ${PAK_DIR} VPK_STAMP_FILES
    # Don't overwrite chapter thumbnails
    -re "^(?!materials\/vgui\/chapters).*"
)

# Extract audio for each selected additional non-English locale
foreach(LANGUAGE ${AUDIO_LANGUAGES_LIST})
    if (LANGUAGE STREQUAL "english")
        # English audio is included in the base VPKs
        continue()
    endif()

    set(LANGUAGE_VPKS
        "portal_sound_vo_${LANGUAGE}_dir.vpk"
        "portal_sound_vo_${LANGUAGE}_000.vpk"
    )
    set(OUTPUT_DIR "${PAK_DIR}/localized/${LANGUAGE}")

    _add_vpk_extract_command("${LANGUAGE_VPKS}" ${OUTPUT_DIR} VPK_STAMP_FILES)
endforeach()

_add_normalize_localized_audio_command(VPK_STAMP_FILES)

add_custom_target(extract_vpks
    DEPENDS ${VPK_STAMP_FILES}
)
set_target_properties(extract_vpks PROPERTIES
    OUTPUTS "${VPK_STAMP_FILES}"
)
