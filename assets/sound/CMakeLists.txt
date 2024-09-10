set(PAK_SOUND_DIR "${PAK_DIR}/sound")
set(PAK_MODIFIED_SOUND_DIR "${PAK_MODIFIED_DIR}/sound")

##########################
## Sound transformation ##
##########################

set(SOUND_SCRIPTS
    ambient/alarms/portal_elevator_chime.sox
    ambient/dinosaur_fizzle.sox
    ambient/machines/portalgun_rotate1.sox
    ambient/machines/ticktock1.sox
    ambient/machines/wall_move5.sox
    buttons/button10.sox
    buttons/button3.sox
    common/wpn_denyselect.sox
    common/wpn_select.sox
    doors/door_metal_thin_close2.sox
    doors/doormove1.jsox
    doors/doormove2.sox
    plats/elevator_move_loop1.sox
    plats/elevator_stop1.sox
    player/footsteps/concrete1.sox
    player/footsteps/concrete2.sox
    player/footsteps/concrete3.sox
    player/footsteps/concrete4.sox
    player/portal_enter1.sox
    player/portal_enter2.sox
    player/portal_exit1.sox
    player/portal_exit2.sox
    player/suit_denydevice.sox
    ui/buttonclickrelease.sox
    ui/buttonrollover.sox
    vehicles/apc/apc_shutdown.sox
    vehicles/apc/apc_start_loop3.jsox
    vehicles/tank_turret_start1.sox
    vo/aperture_ai/00_part1_entry-1.sox
    vo/aperture_ai/00_part1_entry-2.sox
    vo/aperture_ai/00_part1_entry-3.sox
    vo/aperture_ai/00_part1_entry-4.sox
    vo/aperture_ai/00_part1_entry-5.sox
    vo/aperture_ai/00_part1_entry-6.sox
    vo/aperture_ai/00_part1_entry-7.sox
    vo/aperture_ai/00_part1_success-1.sox
    vo/aperture_ai/00_part1_success-2.sox
    vo/aperture_ai/00_part1_success-3.sox
    vo/aperture_ai/00_part2_entry-1.sox
    vo/aperture_ai/00_part2_success-1.sox
    vo/aperture_ai/01_part1_entry-1.sox
    vo/aperture_ai/01_part1_entry-2.sox
    vo/aperture_ai/01_part1_get_portal_gun-1.sox
    vo/aperture_ai/01_part1_get_portal_gun-2.sox
    vo/aperture_ai/01_part1_get_portal_gun-3.sox
    vo/aperture_ai/01_part1_get_portal_gun-4.sox
    vo/aperture_ai/01_part1_get_portal_gun-5.sox
    vo/aperture_ai/01_part1_get_portal_gun-6.sox
    vo/aperture_ai/01_part1_get_portal_gun-7.sox
    vo/aperture_ai/01_part1_get_portal_gun-8.sox
    vo/aperture_ai/01_part2_entry-1.sox
    vo/aperture_ai/01_part2_success-1.sox
    vo/aperture_ai/02_part1_entry-1.sox
    vo/aperture_ai/02_part1_entry-2.sox
    vo/aperture_ai/02_part1_success-1.sox
    vo/aperture_ai/02_part1_success-2.sox
    vo/aperture_ai/02_part2_success-1.sox
    vo/aperture_ai/02_part2_success-2.sox
    vo/aperture_ai/03_part1_entry-1.sox
    vo/aperture_ai/03_part1_entry-2.sox
    vo/aperture_ai/03_part1_success-1.sox
    vo/aperture_ai/03_part2_entry-1.sox
    vo/aperture_ai/03_part2_platform_activated-1.sox
    vo/aperture_ai/04_part1_entry-1.sox
    vo/aperture_ai/04_part1_success-1.sox
    vo/aperture_ai/05_part1_entry-1.sox
    vo/aperture_ai/05_part1_entry-2.sox
    vo/aperture_ai/05_part1_nag1-1.sox
    vo/aperture_ai/05_part1_nag2-1.sox
    vo/aperture_ai/05_part1_nag3-1.sox
    vo/aperture_ai/05_part1_nag4-1.sox
    vo/aperture_ai/05_part1_nag5-1.sox
    vo/aperture_ai/05_part1_success-1.sox
    vo/aperture_ai/06_part1_entry-1.sox
    vo/aperture_ai/06_part1_success_1-1.sox
    vo/aperture_ai/06_part1_success_2-1.sox
    vo/aperture_ai/07_part1_entry-1.sox
    vo/aperture_ai/07_part1_entry-2.sox
    vo/aperture_ai/07_part1_entry-3.sox
    vo/aperture_ai/07_part1_get_device_component-1.sox
    vo/aperture_ai/07_part1_get_device_component-2.sox
    vo/aperture_ai/07_part1_get_device_component-3.sox
    vo/aperture_ai/07_part1_trapped-1.sox
    vo/aperture_ai/07_part1_trapped-2.sox
    vo/aperture_ai/07_part2_entry-1.sox
    vo/aperture_ai/07_part2_success-1.sox
    vo/aperture_ai/08_part1_entry-1.sox
    vo/aperture_ai/08_part1_entry-2.sox
    vo/aperture_ai/08_part1_entry-3.sox
    vo/aperture_ai/08_part1_success-1.sox
    vo/aperture_ai/08_part1_success-2.sox
    vo/aperture_ai/08_part1_trapped-1.sox
    vo/aperture_ai/08_part1_trapped-2.sox
    vo/aperture_ai/09_part1_entry-1.sox
    vo/aperture_ai/09_part1_entry-2.sox
    vo/aperture_ai/09_part1_success-1.sox
    vo/aperture_ai/10_part1_entry-1.sox
    vo/aperture_ai/10_part1_entry-2.sox
    vo/aperture_ai/10_part1_entry-3.sox
    vo/aperture_ai/10_part1_success-1.sox
    vo/aperture_ai/ding_off.sox
    vo/aperture_ai/ding_on.sox
    vo/aperture_ai/escape_01_part1_nag01-1.sox
    vo/aperture_ai/generic_crate_vaporized_in_emancipation_grid-1.sox
    vo/aperture_ai/generic_crate_vaporized_in_emancipation_grid-2.sox
    vo/aperture_ai/generic_security_camera_destroyed-1.sox
    vo/aperture_ai/generic_security_camera_destroyed-2.sox
    vo/aperture_ai/generic_security_camera_destroyed-3.sox
    vo/aperture_ai/generic_security_camera_destroyed-4.sox
    vo/aperture_ai/generic_security_camera_destroyed-5.sox
    weapons/cguard/charging.sox
    weapons/physcannon/energy_bounce1.sox
    weapons/physcannon/energy_disintegrate4.sox
    weapons/physcannon/energy_sing_explosion2.sox
    weapons/physcannon/energy_sing_flyby1.sox
    weapons/portalgun/portal_fizzle2.sox
    weapons/portalgun/portal_open2.sox
    weapons/portalgun/portalgun_shoot_blue1.sox
    weapons/portalgun/portalgun_shoot_red1.sox
    weapons/stunstick/alyx_stunner1.sox
)

# TODO: rename to .sox after makefile is no longer in use
set(MUSIC_SCRIPTS
    music/portal_procedural_jiggle_bone.msox
    music/portal_self_esteem_fund.msox
    music/portal_still_alive.msox
    music/portal_subject_name_here.msox
    music/portal_taste_of_blood.msox
)

# TODO: special cases:
#   - valve (extract from video)
#   - tank_turret_loop1 (generate wav with 16 bits per sample - original has 8)
#   - ambience_base (generate wav at 22050 kHz - original is 11025)

function(_add_sound_transform_command_sox SOUND_SCRIPT INPUT_FILE OUTPUT_FILE)
    add_custom_command(
        DEPENDS
            extract_vpks ${CONVERT_ASSET} ${INPUT_FILE} ${SOUND_SCRIPT}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${PYTHON3} ${CONVERT_ASSET} ${SOX} ${INPUT_FILE} ${SOUND_SCRIPT} ${OUTPUT_FILE}
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

function(_add_sound_transform_command_jsox SOUND_SCRIPT INPUT_FILE OUTPUT_FILE)
    add_custom_command(
        DEPENDS
            extract_vpks ${JSOX} ${INPUT_FILE} ${SOUND_SCRIPT}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${NODEJS} ${JSOX} ${SOUND_SCRIPT} ${INPUT_FILE} ${OUTPUT_FILE}
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

function(_add_sound_transform_command SOUND_SCRIPT OUTPUT_LIST)
    cmake_path(
        REMOVE_EXTENSION SOUND_SCRIPT
        OUTPUT_VARIABLE SOUND_NAME
    )
    cmake_path(
        RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_DIR
    )

    # TODO: change output to PAK_MODIFIED_SOUND_DIR once makefile is no longer in use
    set(SOUND_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${SOUND_SCRIPT}")
    set(INPUT_FILE "${PAK_SOUND_DIR}/${SOUND_NAME}.wav")
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_DIR}/${SOUND_NAME}.wav")

    if(SOUND_SCRIPT MATCHES ".*\.jsox")
        _add_sound_transform_command_jsox(${SOUND_SCRIPT} ${INPUT_FILE} ${OUTPUT_FILE})
    else()
        _add_sound_transform_command_sox(${SOUND_SCRIPT} ${INPUT_FILE} ${OUTPUT_FILE})
    endif()

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

function(_add_music_transform_command MUSIC_SCRIPT OUTPUT_LIST)
    cmake_path(
        REMOVE_EXTENSION MUSIC_SCRIPT
        OUTPUT_VARIABLE MUSIC_NAME
    )
    cmake_path(
        RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_DIR
    )

    set(MUSIC_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${MUSIC_SCRIPT}")
    set(INPUT_FILE "${PAK_SOUND_DIR}/${MUSIC_NAME}.mp3")
    set(CONVERTED_FILE "${PAK_SOUND_DIR}/${MUSIC_NAME}.wav")
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_DIR}/${MUSIC_NAME}.wav")

    # First convert to WAV
    add_custom_command(
        DEPENDS
            extract_vpks ${INPUT_FILE}
        OUTPUT
            ${CONVERTED_FILE}
        COMMAND
            ${MPG123} -q -w ${CONVERTED_FILE} ${INPUT_FILE}
        COMMENT
            "Converting $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    # Now we can process the WAV
    _add_sound_transform_command_sox(${MUSIC_SCRIPT} ${CONVERTED_FILE} ${OUTPUT_FILE})

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for transforming audio files
# Music is stored as MP3 and is converted to WAV first

set(SOUNDS_TRANSFORMED "")

foreach(SOUND_SCRIPT ${SOUND_SCRIPTS})
    _add_sound_transform_command(${SOUND_SCRIPT} SOUNDS_TRANSFORMED)
endforeach()

foreach(MUSIC_SCRIPT ${MUSIC_SCRIPTS})
    _add_music_transform_command(${MUSIC_SCRIPT} SOUNDS_TRANSFORMED)
endforeach()

add_custom_target(
    all_sounds
    DEPENDS ${SOUNDS_TRANSFORMED}
)

# TODO: convert to AIFC
