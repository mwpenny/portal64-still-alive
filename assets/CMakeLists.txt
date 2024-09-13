set(VALVE_INTRO_VIDEO "${VPK_DIR}/Portal/hl2/media/valve.bik")

if(NOT EXISTS ${VALVE_INTRO_VIDEO})
    # The Mac release of Portal uses a .mov instead of a .bik
    string(REPLACE ".mov" ".bik" VALVE_INTRO_VIDEO_MAC "${VALVE_INTRO_VIDEO}")

    # The Valve video doesn't exist in a VPK so we can check its type at
    # configuration time and avoid complicating the conversion commands.
    #
    # Users may forget to copy their game files before running CMake.
    # Only use the .mov if it exists at configuration time, so the build
    # system supports the common case by default.
    if (EXISTS ${VALVE_INTRO_VIDEO_MAC})
        set(VALVE_INTRO_VIDEO ${VALVE_INTRO_VIDEO_MAC})
    endif()
endif()

add_subdirectory(materials)
add_subdirectory(sound)
add_subdirectory(test_chambers)
