# Models

Game object 3D models and predefined animations are created using Blender. Model
files are located in the `assets/models/` directory. Dimentions are at 1/64
scale of the original game.

Model vertex colors and UV coordinates are used. Materials are only used for
their names to look up corresponding material definitions in an `.skm.yaml` file
(see [materials](./materials.md)). Beyond that, they just provide a visual
approximation of what the model will look like in the game.

Final model output currently consists of libultra display lists, any referenced
data such as animation armature and keyframe data (if applicable).

## Model Export

The first step of model conversion is to export the `.blend` files to a form
that is easier to parse. For this, the
[`tools/models/export_fbx.py`](../../tools/models/export_fbx.py) script uses
the Blender Python API to export to the FBX file format, which is commonly
supported across various 3D applications and libraries. Exported models are
saved to `<build directory>/assets/models/`.

## Model Generation

Once exported, models are passed to [Skeletool64](../../skeletool64), which
uses the [assimp](http://www.assimp.org/) library to read them and generate C
code containing model data.

The arguments to pass to Skeletool64 are stored in `.flags` files in
`assets/models/` alongside the corresponding `.blend` files. Generated code is
output to `<build directory>/codegen/assets/models/` at the same relative path
as the input model.

For example:
* Input model: `assets/models/cube/cube.blend`
* Arguments file: `assets/models/cube/cube.flags`.
* Output directory: `<build directory>/codegen/assets/models/cube/`

The arguments file can specify a default material which will be assumed to be
applied before the model is rendered, and therefore excluded from the generated
code. This is useful for reducing material switching (e.g., game loads button
material and then renders all buttons without needing to switch).

At a minimum, the following two files files are generated:
* `<model>_geo.c`: Contains data for model mesh, non-default materials, and
                   armature bones (if applicable).
* `<model>.h`:     Contains `extern` declarations for content of `<model>_geo.c`,
                   and constants for armature bone IDs (for accessing them
                   programmatically).

If a model file contains animations then a third file, `<model>_anim.c`, is
generated which contains animation keyframe data. It is possible for a model
to contain armature data but not animation data (and therefore no `_anim.c`
file). For example, the security camera model contains an armature but is
animated programmatically based on the player's position.

## Dynamic Model Lookup Tables

Some models are common across the entire game. For example, the player and
portal gun. These models are always kept in memory. Other models are less common
and level specific, such as the cube and energy ball launcher/catcher. To save
memory, these kinds of models are only loaded
when playing a level that requires them.

As part of model processing, lookup tables are generated to allow the game to
refer to dynamic models at runtime and load their data (e.g., display lists,
armatures, etc.).

This information is output by
[`tools/models/generate_dynamic_model_list.js`](../../tools/models/generate_dynamic_model_list.js)
and
[`tools/models/generate_dynamic_animated_model_list.js`](../../tools/models/generate_dynamic_animated_model_list.js)
to generated C source files in `<build directory>/codegen/assets/models/`.
