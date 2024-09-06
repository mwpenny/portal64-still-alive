# TODO: multiple languages
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

set(VPK_LISTS
    VPKS_PORTAL
    VPKS_HL2_MISC
    VPKS_HL2_SOUND_MISC
)

set(VPK_STAMP_FILES "")

foreach(VPK_LIST ${VPK_LISTS})
    # First VPK is the one to extract
    list(GET ${VPK_LIST} 0 MAIN_VPK)

    cmake_path(GET MAIN_VPK STEM MAIN_VPK_NAME)
    set(MAIN_VPK_STAMP "${PAK_DIR}/${MAIN_VPK_NAME}.stamp")
    list(APPEND VPK_STAMP_FILES ${MAIN_VPK_STAMP})

    add_custom_command(
        DEPENDS
            ${${VPK_LIST}}
        OUTPUT
            ${MAIN_VPK_STAMP}
        COMMAND
            ${VPK} -x ${PAK_DIR} ${MAIN_VPK}
        COMMAND
            ${CMAKE_COMMAND} -E touch ${MAIN_VPK_STAMP}
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT
            "Extracting ${MAIN_VPK}"
        VERBATIM
    )
endforeach()

add_custom_target(
    extract_vpks
    DEPENDS ${VPK_STAMP_FILES}
)
