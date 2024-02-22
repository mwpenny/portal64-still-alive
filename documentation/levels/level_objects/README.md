# Level Objects

Objects can be placed in a level via their associated
[.blend and .yaml files](../file_formats.md), causing them to be rendered and
animated properly by the game at the correct position/orientation and with the
correct materials applied.

However, the game needs to be able to give each object the proper behavior.
Buttons activate specific doors, triggers start relevant cutscenes, the floor
stops the player from falling through, and so on.

## Naming convention

At level export time, level definition C source files are generated using the
Lua scripts in `tools/level_scripts/`, initiated by `export_level.lua`. The way
the level exporter knows how to properly populate these definitions with objects
is by using their names. Every object in a level's `.blend` file has a name of
the form:
```
@TYPE [ARG]...
```

That is, an `@`-prefixed type name followed by 0 or more space-separated
arguments. The number and type of the arguments is object type specific.

This could have been done with Blender's custom object properties feature, but
one nice benefit of this approach is that it makes searching for related objects
easy.

## Object types

See the linked pages below for details on specific level objects.

| Type                                     | Description                                          |
| ---------------------------------------- | ---------------------------------------------------- |
| [@ambient](./ambient.md)                 | Ambient light source for baked lighting              |
| [@anim](./anim.md)                       | Animatable armature bone                             |
| [@ball_catcher](./ball_catcher.md)       | High energy pellet receptacle                        |
| [@ball_launcher](./ball_launcher.md)     | High energy pellet spawner                           |
| [@box_dropper](./box_dropper.md)         | Cube spawner                                         |
| [@button](./button.md)                   | Flat, circular button                                |
| [@clock](./clock.md)                     | Countdown clock                                      |
| [@collision](./collision.md)             | Static collision                                     |
| [@decor](./decor.md)                     | Generic dynamic object                               |
| [@door](./door.md)                       | Sliding door                                         |
| [@doorway](./doorway.md)                 | Level segment connection for rendering and collision |
| [@dynamic_box](./dynamic_box.md)         | Dynamic collision                                    |
| [@elevator](./elevator.md)               | Level transition elevator                            |
| [@fizzler](./fizzler.md)                 | Emancipation grill                                   |
| [@location](./location.md)               | Referenceable point in the world                     |
| [@pedestal](./pedestal.md)               | Portal gun holder                                    |
| [@point_light](./point_light.md)         | Omnidirectional light source for baked lighting      |
| [@room](./room.md)                       | Discrete level segment for rendering and collision   |
| [@security_camera](./security_camera.md) | Wall-mounted camera                                  |
| [@signage](./signage.md)                 | Start of level informational sign                    |
| [@static](./static.md)                   | Static geometry                                      |
| [@switch](./switch.md)                   | Button on pedestal                                   |
| [@trigger](./trigger.md)                 | Signal and cutscene activating volume                |
