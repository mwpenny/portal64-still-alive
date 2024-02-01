# Level Objects

Objects can be placed in a level via their associated
[.blend and .yaml files](../file_formats.md), causing them to be rendered and
animated properly by the game at the correct position/orientation and with the
correct materials applied.

However, the game needs to be able to give each object the proper behavior.
Buttons activate specific doors, triggers start relevant cutscenes, the floor
stops the player from falling through, and so on.

## Naming convention

At build time, level definition C source files are generated using the Lua
scripts in `tools/level_scripts/`, initiated by `export_level.lua`. The way the
level exporter knows how to properly populate these definitions is by using
object names. Every object in a level's `.blend` file has a name of the form:
```
@type [arg1] [arg2] ... [argN]
```

That is, an `@`-prefixed type name followed by 0 or more space-separated
arguments. The number and type of the arguments is object type specific.

This could have been done with Blender's custom object properties feature, but
one nice benefit of this approach is that it makes searching for related objects
easy.

## Object information

See the pages below for details on specific level objects.

* [@button](./button.md)
* [@clock](./clock.md)
* [@switch](./switch.md)
* More... (TODO)
