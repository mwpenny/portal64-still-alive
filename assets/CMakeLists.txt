set(VALVE_INTRO_VIDEO "${VPK_DIR}/Portal/hl2/media/valve.bik")

if (NOT EXISTS ${VALVE_INTRO_VIDEO})
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

# Scaling factor when converting 3D assets
#
# Blender files use meters, exported FBX files use centimeters, so
# this is used to convert back to the original design-time units
set(MODEL_SCALE 0.01)

# Scaling factor for render space
set(SCENE_SCALE 128)

# Tools for asset conversion
set(CONVERT_ASSET "${PROJECT_SOURCE_DIR}/tools/convert_asset.py")
set(GEN_SEGMENT_LD "${PROJECT_SOURCE_DIR}/tools/generate_segment_ld.js")
set(EXPORT_FBX "${PROJECT_SOURCE_DIR}/tools/models/export_fbx.py")
set(MODEL_LIST_UTILS "${PROJECT_SOURCE_DIR}/tools/models/model_list_utils.js")

add_subdirectory(materials)
add_subdirectory(models)
add_subdirectory(sound)
add_subdirectory(test_chambers)
add_subdirectory(translations)

# For linking model and level animations
set(ANIMS_LINKER_SCRIPT "${LINKER_SCRIPT_DIR}/anims.ld")
add_custom_command(
    DEPENDS
        ${GEN_SEGMENT_LD}
    OUTPUT
        ${ANIMS_LINKER_SCRIPT}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_SEGMENT_LD}
        --single-segment-name animation_segment
        ${ANIMS_LINKER_SCRIPT}
        0x0D000000
        "$<LIST:FILTER,$<TARGET_OBJECTS:models_dynamic>,INCLUDE,_anim\..+$>"
        "$<LIST:FILTER,$<TARGET_OBJECTS:levels>,INCLUDE,_anim\..+$>"
    COMMENT
        "Generating $<PATH:RELATIVE_PATH,${ANIMS_LINKER_SCRIPT},${PROJECT_SOURCE_DIR}>"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(anims_linker_script
    DEPENDS ${ANIMS_LINKER_SCRIPT}
)
