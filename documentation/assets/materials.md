# Materials

Materials define the visual properties of game objects. They can affect
everything from texturing to color blending to lighting.

Material information is specified in `.skm.yaml` files located in the
`assets/materials/` directory. A single YAML file can specify multiple
materials, each with properties such as texture (if applicable), color, as well
as other N64 RDP/RSP settings. The material schema is defined in
[`skeletool64/schema/material-schema.json`](../../skeletool64/schema/material-schema.json).
In short, it provides a wrapper for libultra display list creation. Since one of
the goals of this project is to remove the requirement for libultra, it is
currently not a priority to document the format in detail.

Final material output is C code currently consisting of libultra display lists
and any referenced data such as textures, palettes, etc.

## Texture Extraction and Conversion

Not every material uses textures, but most that do require the textures to be
extracted from the original game's files and converted. Such textures are stored
in [VTF](https://developer.valvesoftware.com/wiki/VTF_(Valve_Texture_Format))
files (Valve Texture Format) inside
[VPK](https://developer.valvesoftware.com/wiki/VPK_(file_format)) archives.
See the page on [original game files](./original_game_files.md) for details on
VPK extraction.

After the VPK archives are extracted, VTF textures are converted
to PNGs using [vtf2png](https://github.com/eXeC64/vtf2png) and stored alongside
the originals in `portal_pak_dir/`.

Some VTFs have multiple frames and require special handling to generate more
than one image (e.g., indicator lights on/off variants, countdown clock digits).
Similarly, the Valve logo image is created by extracting a frame from the intro
video file using [FFmpeg](https://www.ffmpeg.org/). In both cases the images
are output to `portal_pak_dir/` as well.

At this point, nothing has been modified - just converted.

## Texture Transformation

After Portal textures are converted to easy-to-use PNGs, they are modified to
better fit within N64 limitations. For example, resolution is reduced, some
smaller textures are merged into one, and some large textures are split.

Modification is done using [ImageMagick](https://imagemagick.org/). The
arguments to pass are stored in `.ims` (**I**mage**M**agick **s**cript) files in
`assets/materials/` and the resulting textures are output to
`portal_pak_modified/` - each located at the same relative path as the input
texture in `portal_pak_dir/`.

For example:
* Converted texture: `portal_pak_dir/materials/decals/orange_spot.png`
* Arguments file: `assets/materials/decals/orange_spot.ims`.
* Transformed texture: `portal_pak_modified/materials/decals/orange_spot.png`

There are some special cases. Some `.ims` files specify additional input files
than the one they are named after. This is used to combine textures together
to avoid wasting space and to minimize texture switching. An example of this
is `indicator_lights_floor` including `indicator_lights_corner_floor`.

Other `.ims` files specify additional outputs. Some textures are too large and
reducing resolution would result in an unacceptable loss in quality, so they
are split. An example of this is `chell_face` also generating `chell_head`.

At the end of this process `portal_pak_modified/materials/` will contain only
original-game textures actually used by Portal 64.

## Textures Unique to Portal 64

Some textures were created specifically for this project and are not sourced
from the original game. These are stored in `assets/images/` and are already at
the correct resolution with all necessary modifications made.

It is unnecessary to convert or transform such textures and so they are used
as-is.

## Material Generation

After textures are converted and transformed then materials can be generated.

The `hud`, `static`, and `ui` material collections are used often and so they
are always kept in memory when the game is running. On the other hand, materials
in `images` are loaded dynamically. At build time, the corresponding YAML files
are run through Skeletool64 to generate C code containing texture bitmap data
and libultra display lists. These files are output to
`<build directory>/codegen/assets/materials/` and compiled with the rest of the
game. At runtime, the engine executes a given material display list before
drawing geometry that uses the material.

Other material YAML files function more like static libraries during dynamic
model generation. They are not used to generate standalone C code. Instead,
models using non-"base" materials embed the material data in their generated
files so that when the model is loaded/unloaded from memory so are its
materials. For more information see [models](./models.md).
