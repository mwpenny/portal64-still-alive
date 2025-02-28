set(PAK_SOUND_DIR          "${PAK_DIR}/sound")
set(PAK_MODIFIED_SOUND_DIR "${PAK_MODIFIED_DIR}/sound")

set(JSOX                   "${PROJECT_SOURCE_DIR}/tools/sound/jsox.js")
set(GEN_SOUND_IDS          "${PROJECT_SOURCE_DIR}/tools/sound/generate_sound_ids.js")

cmake_path(
    RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
)
set(OUTPUT_DIR "${PROJECT_BINARY_DIR}/${RELATIVE_CURRENT_DIR}")

function(_get_localized_pak_sound_dir LANGUAGE DIR_OUT)
    if (LANGUAGE STREQUAL "english")
        set(${DIR_OUT} "${PAK_SOUND_DIR}")
    else()
        set(${DIR_OUT} "${PAK_DIR}/localized/${LANGUAGE}/sound")
    endif()

    return(PROPAGATE ${DIR_OUT})
endfunction()

function(_get_localized_sound_name SOUND_NAME LANGUAGE NAME_OUT)
    cmake_path(
        GET SOUND_NAME FILENAME
        SOUND_FILENAME
    )
    cmake_path(
        REPLACE_FILENAME SOUND_NAME
        "${LANGUAGE}_${SOUND_FILENAME}"
        OUTPUT_VARIABLE ${NAME_OUT}
    )

    return(PROPAGATE ${NAME_OUT})
endfunction()

#################
## Valve sound ##
#################

function(_add_extract_video_audio_command INPUT_FILE OUTPUT_FILE)
    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>"
        COMMAND
            ${FFmpeg_EXECUTABLE}
            -i ${INPUT_FILE} -vn -loglevel error -y ${OUTPUT_FILE}
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
    ambient/atmosphere/ambience_base.sox
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
    npc/turret_floor/active.sox
    npc/turret_floor/deploy.sox
    npc/turret_floor/ping.sox
    npc/turret_floor/retract.sox
    npc/turret_floor/shoot1.jsox
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
    vehicles/tank_turret_loop1.sox
    vehicles/tank_turret_start1.sox
    vo/aperture_ai/ding_off.sox
    vo/aperture_ai/ding_on.sox
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

set(LOCALIZED_SOUND_SCRIPTS
    npc/turret_floor/turret_disabled_2.sox
    npc/turret_floor/turret_disabled_4.sox
    npc/turret_floor/turret_disabled_5.sox
    npc/turret_floor/turret_disabled_6.sox
    npc/turret_floor/turret_disabled_7.sox
    npc/turret_floor/turret_disabled_8.sox
    npc/turret_floor/turret_search_1.sox
    npc/turret_floor/turret_search_2.sox
    npc/turret_floor/turret_search_4.sox
    npc/turret_floor/turret_fizzler_1.sox
    npc/turret_floor/turret_pickup_1.sox
    npc/turret_floor/turret_pickup_3.sox
    npc/turret_floor/turret_pickup_6.sox
    npc/turret_floor/turret_pickup_7.sox
    npc/turret_floor/turret_pickup_8.sox
    npc/turret_floor/turret_pickup_9.sox
    npc/turret_floor/turret_pickup_10.sox
    npc/turret_floor/turret_retire_1.sox
    npc/turret_floor/turret_retire_2.sox
    npc/turret_floor/turret_retire_4.sox
    npc/turret_floor/turret_retire_5.sox
    npc/turret_floor/turret_retire_6.sox
    npc/turret_floor/turret_retire_7.sox
    npc/turret_floor/turret_tipped_2.sox
    npc/turret_floor/turret_tipped_4.sox
    npc/turret_floor/turret_tipped_5.sox
    npc/turret_floor/turret_tipped_6.sox
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
    vo/aperture_ai/escape_01_part1_nag01-1.sox
    vo/aperture_ai/generic_crate_vaporized_in_emancipation_grid-1.sox
    vo/aperture_ai/generic_crate_vaporized_in_emancipation_grid-2.sox
    vo/aperture_ai/generic_security_camera_destroyed-1.sox
    vo/aperture_ai/generic_security_camera_destroyed-2.sox
    vo/aperture_ai/generic_security_camera_destroyed-3.sox
    vo/aperture_ai/generic_security_camera_destroyed-4.sox
    vo/aperture_ai/generic_security_camera_destroyed-5.sox
)

# Localized sounds which are not localized in this language
set(RUSSIAN_EXCLUDED_LOCALIZED_SOUND_SCRIPTS
    npc/turret_floor/turret_disabled_4.sox
    npc/turret_floor/turret_disabled_5.sox
    npc/turret_floor/turret_disabled_6.sox
    npc/turret_floor/turret_disabled_7.sox
    npc/turret_floor/turret_disabled_8.sox
    npc/turret_floor/turret_search_2.sox
    npc/turret_floor/turret_search_4.sox
    npc/turret_floor/turret_pickup_3.sox
    npc/turret_floor/turret_pickup_6.sox
    npc/turret_floor/turret_pickup_7.sox
    npc/turret_floor/turret_pickup_8.sox
    npc/turret_floor/turret_pickup_9.sox
    npc/turret_floor/turret_pickup_10.sox
    npc/turret_floor/turret_retire_2.sox
    npc/turret_floor/turret_retire_4.sox
    npc/turret_floor/turret_retire_5.sox
    npc/turret_floor/turret_retire_6.sox
    npc/turret_floor/turret_retire_7.sox
    npc/turret_floor/turret_tipped_4.sox
    npc/turret_floor/turret_tipped_5.sox
    npc/turret_floor/turret_tipped_6.sox
)

set(MUSIC_SCRIPTS
    music/portal_procedural_jiggle_bone.sox
    music/portal_self_esteem_fund.sox
    music/portal_still_alive.sox
    music/portal_subject_name_here.sox
    music/portal_taste_of_blood.sox
)

set(INSTRUMENT_BANKS
    ambient/atmosphere/ambience_base.ins
    ambient/machines/beam_platform_loop1.ins
    ambient/music/looping_radio_mix.ins
    doors/door_metal_medium_open1.ins
    npc/turret_floor/shoot1.ins
    vehicles/fast_windloop1.ins
    vehicles/tank_turret_loop1.ins
    weapons/physcannon/energy_sing_loop4.ins
)

function(_add_sound_transform_command_sox SOUND_SCRIPT INPUT_FILE OUTPUT_FILE)
    get_source_file_property(IS_GENERATED_INPUT ${INPUT_FILE} GENERATED)

    add_custom_command(
        DEPENDS
            extract_vpks "$<TARGET_PROPERTY:extract_vpks,OUTPUTS>"
            "$<$<BOOL:${IS_GENERATED_INPUT}>:${INPUT_FILE}>"
            ${RUN_COMMAND}
            ${SOUND_SCRIPT}
        OUTPUT
            ${OUTPUT_FILE}
        WORKING_DIRECTORY
            # Allow scripts to easily reference other sounds
            "$<PATH:GET_PARENT_PATH,${INPUT_FILE}>"
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE}>"
        COMMAND
            ${Python3_EXECUTABLE} ${RUN_COMMAND}
            ${Sox_EXECUTABLE} ${INPUT_FILE} @${SOUND_SCRIPT} ${OUTPUT_FILE} -V1
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

function(_add_sound_transform_command_jsox SOUND_SCRIPT INPUT_FILE OUTPUT_FILE)
    add_custom_command(
        DEPENDS
            extract_vpks "$<TARGET_PROPERTY:extract_vpks,OUTPUTS>"
            ${JSOX}
            ${SOUND_SCRIPT}
        OUTPUT
            ${OUTPUT_FILE}
        WORKING_DIRECTORY
            # Allow scripts to easily reference other sounds
            "$<PATH:GET_PARENT_PATH,${INPUT_FILE}>"
        COMMAND
            ${NodeJs_EXECUTABLE} ${JSOX}
            ${Sox_EXECUTABLE} ${SOUND_SCRIPT} ${INPUT_FILE} ${OUTPUT_FILE}
        COMMENT
            "Transforming $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()

function(_add_sound_transform_command SOUND_SCRIPT INPUT_FILE OUTPUT_NAME OUTPUT_LIST)
    set(SOUND_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${SOUND_SCRIPT}")
    set(OUTPUT_FILE "${PAK_MODIFIED_SOUND_DIR}/${OUTPUT_NAME}.wav")

    if (SOUND_SCRIPT MATCHES ".*\.jsox")
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

    set(MUSIC_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${MUSIC_SCRIPT}")
    set(INPUT_FILE "${PAK_SOUND_DIR}/${MUSIC_NAME}.mp3")
    set(CONVERTED_FILE "${PAK_SOUND_DIR}/${MUSIC_NAME}.wav")
    set(OUTPUT_FILE "${PAK_MODIFIED_SOUND_DIR}/${MUSIC_NAME}.wav")

    # First convert to WAV
    add_custom_command(
        DEPENDS
            extract_vpks "$<TARGET_PROPERTY:extract_vpks,OUTPUTS>"
        OUTPUT
            ${CONVERTED_FILE}
        COMMAND
            ${FFmpeg_EXECUTABLE}
            -i ${INPUT_FILE} -loglevel error -y ${CONVERTED_FILE}
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

foreach(SOUND_SCRIPT ${SOUND_SCRIPTS})
    cmake_path(
        REMOVE_EXTENSION SOUND_SCRIPT
        OUTPUT_VARIABLE SOUND_NAME
    )

    set(INPUT_FILE "${PAK_SOUND_DIR}/${SOUND_NAME}.wav")
    _add_sound_transform_command(${SOUND_SCRIPT} ${INPUT_FILE} ${SOUND_NAME} SOUNDS_TRANSFORMED)
endforeach()

# Localized sounds
# Sounds within a language must be contiguous, so loop over languages first
foreach(LANGUAGE ${AUDIO_LANGUAGES_LIST})
    foreach(SOUND_SCRIPT ${LOCALIZED_SOUND_SCRIPTS})
        string(TOUPPER "${LANGUAGE}_EXCLUDED_LOCALIZED_SOUND_SCRIPTS" EXCLUDED_SOUNDS)
        if (EXCLUDED_SOUNDS AND (${SOUND_SCRIPT} IN_LIST ${EXCLUDED_SOUNDS}))
            continue()
        endif()

        cmake_path(
            REMOVE_EXTENSION SOUND_SCRIPT
            OUTPUT_VARIABLE SOUND_NAME
        )

        _get_localized_pak_sound_dir(${LANGUAGE} INPUT_DIR)
        _get_localized_sound_name(${SOUND_NAME} ${LANGUAGE} LOCALIZED_NAME)
        set(INPUT_FILE "${INPUT_DIR}/${SOUND_NAME}.wav")

        _add_sound_transform_command(${SOUND_SCRIPT} ${INPUT_FILE} ${LOCALIZED_NAME} SOUNDS_TRANSFORMED)
    endforeach()
endforeach()

# Music
foreach(MUSIC_SCRIPT ${MUSIC_SCRIPTS})
    _add_music_transform_command(${MUSIC_SCRIPT} SOUNDS_TRANSFORMED)
endforeach()

######################
## Sound conversion ##
######################

function(_add_sound_convert_command INPUT_FILE OUTPUT_LIST)
    cmake_path(
        RELATIVE_PATH INPUT_FILE
        BASE_DIRECTORY "${PAK_MODIFIED_SOUND_DIR}"
        OUTPUT_VARIABLE SOUND_NAME
    )
    cmake_path(
        REMOVE_EXTENSION SOUND_NAME
        OUTPUT_VARIABLE SOUND_NAME
    )
    set(OUTPUT_FILE "${OUTPUT_DIR}/${SOUND_NAME}.aifc")

    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE}
        COMMAND
            ${SFZ2N64_EXECUTABLE} -o ${OUTPUT_FILE} ${INPUT_FILE}
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

add_custom_target(sounds
    DEPENDS ${SOUNDS_CONVERTED}
)

############################
## Sound table generation ##
############################

# The dependency on extract_vpks covers unconverted sounds,
# so we only need to explicitly specify converted ones
set(SOUND_TABLE_DEPENDENCIES ${SOUNDS_CONVERTED})
set(SOUND_TABLE_INPUTS "")

foreach(INS_BANK ${INSTRUMENT_BANKS})
    set(INS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${INS_BANK}")

    list(APPEND SOUND_TABLE_DEPENDENCIES ${INS_PATH})
    list(APPEND SOUND_TABLE_INPUTS       ${INS_PATH})
endforeach()

foreach(CONVERTED_SOUND ${SOUNDS_CONVERTED})
    cmake_path(
        RELATIVE_PATH CONVERTED_SOUND
        BASE_DIRECTORY ${OUTPUT_DIR}
        OUTPUT_VARIABLE INS_BANK
    )
    cmake_path(
        REPLACE_EXTENSION INS_BANK ".ins"
        OUTPUT_VARIABLE INS_BANK
    )

    # Only include the sound if an instrument bank doesn't already do so
    if (NOT INS_BANK IN_LIST INSTRUMENT_BANKS)
        list(APPEND SOUND_TABLE_INPUTS ${CONVERTED_SOUND})
    endif()
endforeach()

# Avoid extremely long argument list (MSYS2 has low ARG_MAX)
list(TRANSFORM SOUND_TABLE_INPUTS REPLACE "${OUTPUT_DIR}/" "" OUTPUT_VARIABLE SOUND_TABLE_INPUTS)

# Add commands for generating sound tables

set(SOUND_TABLE_SOUNDS "${OUTPUT_DIR}/sounds.sounds")
set(SOUND_TABLE_TBL "${OUTPUT_DIR}/sounds.sounds.tbl")
set(SOUND_DATA_TABLE_FILES
    ${SOUND_TABLE_SOUNDS}
    ${SOUND_TABLE_TBL}
)
add_custom_command(
    DEPENDS
        extract_vpks "$<TARGET_PROPERTY:extract_vpks,OUTPUTS>"
        sounds ${SOUND_TABLE_DEPENDENCIES}
    OUTPUT
        ${SOUND_DATA_TABLE_FILES}
    COMMAND
        ${SFZ2N64_EXECUTABLE} -o ${SOUND_TABLE_SOUNDS} ${SOUND_TABLE_INPUTS}
    WORKING_DIRECTORY
        ${OUTPUT_DIR}
    COMMENT
        "Generating sound data tables"
    VERBATIM
)

set(BUILD_AUDIO_DIR "${GENERATED_CODE_DIR}/assets/audio")
set(CLIPS_H "${BUILD_AUDIO_DIR}/clips.h")
set(LANGUAGES_H "${BUILD_AUDIO_DIR}/languages.h")
set(LANGUAGES_C "${BUILD_AUDIO_DIR}/languages.c")
set(SOUND_LOOKUP_TABLE_FILES
    ${CLIPS_H}
    ${LANGUAGES_H}
    ${LANGUAGES_C}
)
add_custom_command(
    DEPENDS
        extract_vpks "$<TARGET_PROPERTY:extract_vpks,OUTPUTS>"
        sounds ${SOUND_TABLE_DEPENDENCIES}
        ${GEN_SOUND_IDS}
    OUTPUT
        ${SOUND_LOOKUP_TABLE_FILES}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_SOUND_IDS}
        --out-dir ${BUILD_AUDIO_DIR} ${SOUND_TABLE_INPUTS}
    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}
    COMMENT
        "Generating sound lookup tables"
    VERBATIM
)

add_custom_target(sound_tables
    DEPENDS
        ${SOUND_DATA_TABLE_FILES}
        ${SOUND_LOOKUP_TABLE_FILES}
)

add_custom_target(sound_data_tables
    DEPENDS
        sound_tables
        ${SOUND_DATA_TABLE_FILES}
)
set_target_properties(sound_data_tables PROPERTIES
    OUTPUTS "${SOUND_DATA_TABLE_FILES}"
)

add_library(sound_lookup_tables INTERFACE)
add_dependencies(sound_lookup_tables sound_tables)
target_sources(sound_lookup_tables INTERFACE
    ${SOUND_LOOKUP_TABLE_FILES}
)
