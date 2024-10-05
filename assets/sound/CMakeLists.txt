# TODO: multiple languages

set(PAK_SOUND_DIR "${PAK_DIR}/sound")
set(PAK_MODIFIED_SOUND_DIR "${PAK_MODIFIED_DIR}/sound")

cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
)

#################
## Valve sound ##
#################

function(_add_extract_video_audio_command INPUT_FILE OUTPUT_FILE)
    cmake_path(
        GET OUTPUT_FILE PARENT_PATH
        OUTPUT_DIR
    )

    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${FFMPEG} -i ${INPUT_FILE} -vn -loglevel error -y ${OUTPUT_FILE}
        COMMENT
            "Extracting audio from $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

_add_extract_video_audio_command(
    ${VALVE_INTRO_VIDEO}
    ${PAK_SOUND_DIR}/ambient/music/valve.wav
)

##########################
## Sound transformation ##
##########################

set(SOUND_SCRIPTS
    ambient/alarms/portal_elevator_chime.sox
    ambient/dinosaur_fizzle.sox
    ambient/machines/portalgun_rotate1.sox
    ambient/machines/ticktock1.sox
    ambient/machines/wall_move5.sox
    ambient/music/valve.sox
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

set(INSTRUMENT_BANKS
    ambient/atmosphere/ambience_base.ins
    ambient/machines/beam_platform_loop1.ins
    ambient/music/looping_radio_mix.ins
    doors/door_metal_medium_open1.ins
    vehicles/fast_windloop1.ins
    vehicles/tank_turret_loop1.ins
    weapons/physcannon/energy_sing_loop4.ins
)

# TODO: remove
function(_add_sound_transform_command_direct SOUND_NAME ARGS OUTPUT_LIST)
    set(INPUT_FILE "${PAK_SOUND_DIR}/${SOUND_NAME}.wav")
    string(REPLACE " " ";" ARGS ${ARGS})

    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${SOUND_NAME}.wav")

    cmake_path(
        GET OUTPUT_FILE PARENT_PATH
        OUTPUT_DIR
    )

    add_custom_command(
        DEPENDS
            extract_vpks ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${SOX} ${INPUT_FILE} ${ARGS} ${OUTPUT_FILE}
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

function(_add_sound_transform_command SOUND_SCRIPT OUTPUT_LIST)
    cmake_path(
        REMOVE_EXTENSION SOUND_SCRIPT
        OUTPUT_VARIABLE SOUND_NAME
    )

    # TODO: change output to PAK_MODIFIED_SOUND_DIR once makefile is no longer in use
    set(SOUND_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${SOUND_SCRIPT}")
    set(INPUT_FILE "${PAK_SOUND_DIR}/${SOUND_NAME}.wav")
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${SOUND_NAME}.wav")

    if(SOUND_SCRIPT MATCHES ".*\.jsox")
        _add_sound_transform_command_jsox(${SOUND_SCRIPT} ${INPUT_FILE} ${OUTPUT_FILE})
    else()
        _add_sound_transform_command_sox(${SOUND_SCRIPT} ${INPUT_FILE} ${OUTPUT_FILE})
    endif()

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

function(_add_sound_transform_command_sox SOUND_SCRIPT INPUT_FILE OUTPUT_FILE)
    cmake_path(
        GET OUTPUT_FILE PARENT_PATH
        OUTPUT_DIR
    )

    add_custom_command(
        DEPENDS
            extract_vpks ${CONVERT_ASSET} ${INPUT_FILE} ${SOUND_SCRIPT}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND
            ${Python3_EXECUTABLE} ${CONVERT_ASSET} ${SOX} ${INPUT_FILE} ${SOUND_SCRIPT} ${OUTPUT_FILE}
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

function(_add_music_transform_command MUSIC_SCRIPT OUTPUT_LIST)
    cmake_path(
        REMOVE_EXTENSION MUSIC_SCRIPT
        OUTPUT_VARIABLE MUSIC_NAME
    )

    # TODO: change output to PAK_MODIFIED_SOUND_DIR once makefile is no longer in use
    set(MUSIC_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${MUSIC_SCRIPT}")
    set(INPUT_FILE "${PAK_SOUND_DIR}/${MUSIC_NAME}.mp3")
    set(CONVERTED_FILE "${PAK_SOUND_DIR}/${MUSIC_NAME}.wav")
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${MUSIC_NAME}.wav")

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

# Add commands for transforming sound files
# Music is stored as MP3 and is converted to WAV first

set(SOUNDS_TRANSFORMED "")

# TODO: remove this special casing once makefile is no longer in use (just use .sox)
_add_sound_transform_command_direct(
    vehicles/tank_turret_loop1
    "-b 16"
    SOUNDS_TRANSFORMED
)
_add_sound_transform_command_direct(
    ambient/atmosphere/ambience_base
    "-c 1 -r 22050"
    SOUNDS_TRANSFORMED
)

foreach(SOUND_SCRIPT ${SOUND_SCRIPTS})
    _add_sound_transform_command(${SOUND_SCRIPT} SOUNDS_TRANSFORMED)
endforeach()

foreach(MUSIC_SCRIPT ${MUSIC_SCRIPTS})
    _add_music_transform_command(${MUSIC_SCRIPT} SOUNDS_TRANSFORMED)
endforeach()

######################
## Sound conversion ##
######################

function(_add_sound_convert_command INPUT_FILE OUTPUT_LIST)
    cmake_path(
        REPLACE_EXTENSION INPUT_FILE
        ".aifc"
        OUTPUT_VARIABLE OUTPUT_FILE
    )

    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${SFZ2N64} -o ${OUTPUT_FILE} ${INPUT_FILE}
        COMMENT
            "Converting $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

# Add commands for converting transformed sound files to N64 AIFC (compressed AIFF)

set(SOUNDS_CONVERTED "")

foreach(TRANSFORMED_SOUND ${SOUNDS_TRANSFORMED})
    _add_sound_convert_command(${TRANSFORMED_SOUND} SOUNDS_CONVERTED)
endforeach()

add_custom_target(
    all_sounds
    DEPENDS ${SOUNDS_CONVERTED}
)

############################
## Sound table generation ##
############################

set(SOUND_TABLE_DEPENDENCIES ${SOUNDS_CONVERTED})
set(SOUND_TABLE_INPUTS "")

foreach(INS_BANK ${INSTRUMENT_BANKS})
    cmake_path(
        REMOVE_EXTENSION INS_BANK
        OUTPUT_VARIABLE SOUND_NAME
    )

    # Each bank references a WAV. Keep track of the dependency.
    #
    # Some banks depend on converted sounds instead of unmodified.
    # That dependency is taken care of via SOUNDS_CONVERTED.
    list(APPEND SOUND_TABLE_DEPENDENCIES "${PAK_SOUND_DIR}/${SOUND_NAME}.wav")
    list(APPEND SOUND_TABLE_INPUTS "${CMAKE_CURRENT_SOURCE_DIR}/${INS_BANK}")
endforeach()

foreach(CONVERTED_SOUND ${SOUNDS_CONVERTED})
    cmake_path(
        RELATIVE_PATH CONVERTED_SOUND
        BASE_DIRECTORY "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}"
        OUTPUT_VARIABLE INS_BANK
    )
    cmake_path(
        REPLACE_EXTENSION INS_BANK
        ".ins"
        OUTPUT_VARIABLE INS_BANK
    )

    # Only include the sound if an instrument bank doesn't already do so
    if(NOT INS_BANK IN_LIST INSTRUMENT_BANKS)
        list(APPEND SOUND_TABLE_INPUTS ${CONVERTED_SOUND})
    endif()
endforeach()

# Add commands for generating sound tables

set(SOUND_TABLE_SOUNDS "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/sounds.sounds")
set(SOUND_TABLE_TBL "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/sounds.sounds.tbl")
add_custom_command(
    DEPENDS
        all_sounds ${SOUND_TABLE_DEPENDENCIES}
    OUTPUT
        ${SOUND_TABLE_SOUNDS} ${SOUND_TABLE_TBL}
    COMMAND
        ${SFZ2N64} -o ${SOUND_TABLE_SOUNDS} ${SOUND_TABLE_INPUTS}
    COMMENT
        "Generating sound data tables"
    VERBATIM
)

set(BUILD_AUDIO_DIR "${CMAKE_BINARY_DIR}/src/audio")
set(CLIPS_H "${BUILD_AUDIO_DIR}/clips.h")
set(LANGUAGES_H "${BUILD_AUDIO_DIR}/languages.h")
set(LANGUAGES_C "${BUILD_AUDIO_DIR}/languages.c")
add_custom_command(
    DEPENDS
        all_sounds ${GEN_SOUND_IDS} ${SOUND_TABLE_DEPENDENCIES}
    OUTPUT
        ${CLIPS_H} ${LANGUAGES_H} ${LANGUAGES_C}
    COMMAND
        ${NODEJS} ${GEN_SOUND_IDS} -o ${BUILD_AUDIO_DIR} -p SOUNDS_ ${SOUND_TABLE_INPUTS}
    COMMENT
        "Generating sound lookup tables"
    VERBATIM
)

add_custom_target(
    sound_tables
    DEPENDS
        # Sound data
        ${SOUND_TABLE_SOUNDS}
        ${SOUND_TABLE_TBL}

        # Lookup tables
        ${CLIPS_H}
        ${LANGUAGES_H}
        ${LANGUAGES_C}
)
