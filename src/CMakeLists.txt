###############
## Game code ##
###############

option(DEBUGGER      "Build with support for hardware debugging")
option(GFX_VALIDATOR "Build with display list validator")
option(RSP_PROFILER  "Build with RSP performance profiler")

add_library(engine INTERFACE)

target_include_directories(engine INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(engine INTERFACE
    platform
)

target_sources(engine INTERFACE
    audio/audio.c
    audio/audiomgr.c
    audio/clips.c
    audio/soundarray.c
    audio/soundplayer.c
    controls/controller_actions.c
    controls/rumble_pak.c
    decor/decor_object.c
    decor/decor_object_list.c
    effects/effect_definitions.c
    effects/effects.c
    effects/portal_trail.c
    effects/splash_particle_effect.c
    font/dejavu_sans_images.c
    font/font.c
    font/liberation_mono_images.c
    graphics/color.c
    graphics/debug_render.c
    graphics/graphics.c
    graphics/image.c
    graphics/initgfx.c
    graphics/profile_task.c
    graphics/render_scene.c
    graphics/renderstate.c
    graphics/screen_clipper.c
    levels/credits.c
    levels/cutscene_runner.c
    levels/intro.c
    levels/level_definition.c
    levels/levels.c
    levels/material_state.c
    levels/static_render.c
    locales/locales.c
    main.c
    materials/light.c
    materials/point_light_rendered.c
    materials/shadow_caster.c
    materials/subject.c
    math/basis.c
    math/box2d.c
    math/box3d.c
    math/boxs16.c
    math/mathf.c
    math/matrix.c
    math/plane.c
    math/quaternion.c
    math/ray.c
    math/rotated_box.c
    math/transform.c
    math/vector2.c
    math/vector2s16.c
    math/vector3.c
    math/vector4.c
    menu/audio_options.c
    menu/cheat_codes.c
    menu/confirmation_dialog.c
    menu/controls.c
    menu/game_menu.c
    menu/gameplay_options.c
    menu/joystick_options.c
    menu/landing_menu.c
    menu/load_game.c
    menu/main_menu.c
    menu/menu.c
    menu/menu_builder.c
    menu/new_game_menu.c
    menu/options_menu.c
    menu/save_game_menu.c
    menu/savefile_list.c
    menu/tabs.c
    menu/text_manipulation.c
    menu/video_options.c
    physics/collision.c
    physics/collision_box.c
    physics/collision_capsule.c
    physics/collision_cylinder.c
    physics/collision_object.c
    physics/collision_point.c
    physics/collision_quad.c
    physics/collision_scene.c
    physics/collision_sphere.c
    physics/collision_tetrahedron.c
    physics/contact_insertion.c
    physics/contact_solver.c
    physics/debug_renderer.c
    physics/epa.c
    physics/gjk.c
    physics/line.c
    physics/compound_collider.c
    physics/mesh_collider.c
    physics/point_constraint.c
    physics/raycasting.c
    physics/rigid_body.c
    physics/world.c
    player/grab_rotation.c
    player/player.c
    player/player_rumble_clips.c
    savefile/checkpoint.c
    savefile/savefile.c
    savefile/scene_serialize.c
    savefile/serializer.c
    scene/ball.c
    scene/ball_catcher.c
    scene/ball_launcher.c
    scene/box_dropper.c
    scene/button.c
    scene/camera.c
    scene/clock.c
    scene/debug_scene.c
    scene/door.c
    scene/dynamic_render_list.c
    scene/dynamic_scene.c
    scene/elevator.c
    scene/fizzler.c
    scene/hud.c
    scene/laser.c
    scene/pedestal.c
    scene/point_light.c
    scene/portal.c
    scene/portal_gun.c
    scene/portal_render.c
    scene/portal_surface.c
    scene/portal_surface_generator.c
    scene/portal_surface_gfx.c
    scene/render_plan.c
    scene/scene.c
    scene/scene_animator.c
    scene/security_camera.c
    scene/shadow_map.c
    scene/shadow_renderer.c
    scene/signage.c
    scene/signals.c
    scene/static_scene.c
    scene/switch.c
    scene/trigger_listener.c
    scene/turret.c
    sk64/skeletool_animator.c
    sk64/skeletool_armature.c
    strings/translations.c
    util/assert.c
    util/dynamic_asset_loader.c
    util/memory.c
    util/profile.c
    util/rom.c
    util/string.c
)

if (DEBUGGER)
    target_sources(engine INTERFACE
        debugger/debug.c
        debugger/usb.c
    )
    target_compile_definitions(engine INTERFACE
        PORTAL64_WITH_DEBUGGER
    )
endif()

if (GFX_VALIDATOR)
    target_sources(engine INTERFACE
        gfxvalidator/command_printer.c
        gfxvalidator/error_printer.c
        gfxvalidator/validator.c
    )
    target_compile_definitions(engine INTERFACE
        PORTAL64_WITH_GFX_VALIDATOR
    )
endif()

if (RSP_PROFILER)
    target_compile_definitions(engine INTERFACE
        PORTAL64_WITH_RSP_PROFILER
    )
endif()

#########################
## Version information ##
#########################

set(GEN_VERSION_HEADER "${PROJECT_SOURCE_DIR}/cmake/GenVersionHeader.cmake")
set(VERSION_H          "${GENERATED_CODE_DIR}/version.h")

add_custom_target(version_header
    DEPENDS
        ${GEN_VERSION_HEADER}
    BYPRODUCTS
        ${VERSION_H}
    COMMAND
        ${CMAKE_COMMAND}
        -D Git_EXECUTABLE=${Git_EXECUTABLE}
        -D OUTPUT_FILE=${VERSION_H}
        -P ${GEN_VERSION_HEADER}
    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}
    VERBATIM
)

# Make sure version header is always up to date
add_dependencies(engine version_header)

###########################
## Library-specific code ##
###########################

target_sources(engine INTERFACE
    system/libultra/controller_libultra.c
    system/libultra/time_libultra.c
)
