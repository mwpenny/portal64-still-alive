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

function(_add_vpk_extract_command VPK_LIST OUTPUT_DIR OUTPUT_LIST)
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
            ${VPK_EXECUTABLE} -x ${OUTPUT_DIR} ${MAIN_VPK}
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
    if ("russian" IN_LIST AUDIO_LANGUAGES_LIST)
        set(AUDIO_FIX_STAMP "${PAK_DIR}/localized_audio_normalization.stamp")
        set(RU_SOUND_DIR "${PAK_DIR}/localized/russian/sound")

        add_custom_command(
            DEPENDS
                ${${STAMP_FILE_LIST}}
            OUTPUT
                ${AUDIO_FIX_STAMP}
            COMMAND
                ${CMAKE_COMMAND} -E rename
                "${RU_SOUND_DIR}/npc/turret_floor/turret_dissolved_1.wav"
                "${RU_SOUND_DIR}/npc/turret_floor/turret_fizzler_1.wav"
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
    endif()
endfunction()

set(VPK_STAMP_FILES "")

_add_vpk_extract_command("${VPKS_PORTAL}"         ${PAK_DIR} VPK_STAMP_FILES)
_add_vpk_extract_command("${VPKS_HL2_MISC}"       ${PAK_DIR} VPK_STAMP_FILES)
_add_vpk_extract_command("${VPKS_HL2_SOUND_MISC}" ${PAK_DIR} VPK_STAMP_FILES)

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
