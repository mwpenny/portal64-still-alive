# Levels

> **Note:** This page focuses on level export and conversion. For details on
> level creation and file contents, see
> [documentation/levels/README.md](../levels/README.md).

Level layout and animations are created using Blender and scripting is done
using YAML files. Both files are located in the
`assets/test_chambers/<level name>/` directory for each level. Dimentions are
at 1/64 scale of the original game.

Level mesh vertex colors and UV coordinates are used. Materials are only used
for their names to look up corresponding material definitions in an `.skm.yaml`
file (see [materials](./materials.md)). Beyond that, they just provide a visual
approximation of what the level will look like in the game.

Final level output currently consists of libultra display lists,
[level object](../levels/level_objects/README.md) metadata,
[cutscene](../levels/cutscenes/README.md) steps, animation armature and keyframe
data (if applicable), as well as data structures for culling and portal surfaces.

## Level Export

The first step of level conversion is to export the `.blend` files to a form
that is easier to parse. For this, the
[`tools/models/export_fbx.py`](../../tools/models/export_fbx.py) script uses
the Blender Python API to export to the FBX file format, which is commonly
supported across various 3D applications and libraries. Exported levels are
saved to the `<build directory>/assets/test_chambers/<level name>/` directory
for each level.

## Level Generation

Once exported, levels are passed to [Skeletool64](../../skeletool64), which
uses the [assimp](http://www.assimp.org/) library to read them and generate C
code containing level data. Generated code is output to the
`<build directory>/codegen/assets/test_chambers/<level name>/` directory
for each level.

At a minimum, the following two files files are generated:
* `<level name>_geo.c`: Contains data for level mesh, objects, cutscenes,
                        armatures (if applicable), and lookup data structures.
* `<level name>.h`:     Contains `extern` declarations for content of
                        `<level name>_geo.c`, and constants for armature bone
                        IDs, [signals](../levels/signals.md), and
                        [locations](../levels/level_objects/location.md)
                        (for accessing them programmatically).

If a level contains animations then a third file, `<level name>_anim.c`, is
generated which contains animation keyframe data.

The generated code does not store material data directly. Level mesh materials
are applied by the engine during rendering and so only a material ID is needed.
Similarly, only level object metadata is stored and the model information is
looked up at runtime.

## Lookup Table

As part of level processing, a lookup table is generated to allow the game to
refer to levels at runtime when loading them.

This information is output by
[`tools/models/generate_level_list.js`](../../tools/models/generate_level_list.js)
to the generated header file
`<build directory>/codegen/assets/test_chambers/level_list.h`.
