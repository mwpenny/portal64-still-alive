# Level File Formats

## Levels

Each level of the game is defined via two files:

1. Blender (`.blend`) file
2. YAML (`.yaml`) file

These files are located in the `assets/test_chambers/<level_name>/` directory
corresponding to a given level.

### Level layout (Blender file)

A level's `.blend` file specifies fundamental 3D elements such as layout and
collision as well as the objects contained within (e.g., cubes, doors,
elevators, etc.) and any animations, such as for moving platforms. These files
can be edited in [Blender](https://www.blender.org/) as with any other `.blend`
file. Reusable objects are defined in their own `.blend` files located in the
`assets/models/` directory and then referenced from the level `.blend` files
which use them.

At export time, level object data beyond position, rotation, etc. is generally
not read from `.blend` files. Rather, objects use a naming convention which
tells the exporter their type and parameters so information like the mesh and
material data can be looked up elsewhere. This naming convention is also what
hooks the objects up to code. To learn more about what can appear in a level and
how to configure it, see [Level Objects](./level_objects/README.md).

### Level scripting (YAML file)

The second file that makes up a level is its `.yaml` file. This file contains
its [cutscene steps](./cutscenes.md) and [signal operators](./signals.md). At build
time, the data in these files is used to generate corresponding C code. See the
linked pages for more details.

## Materials

As mentioned above, material data for objects is not read from their `.blend`
files. This is also true for static level geometry. Instead, all materials are
defined in `.skm.yaml` files located in the `assets/materials/` directory. These
YAML files specify textures, colors, as well as other more complex RDP/RSP
settings. They are currently used at export time to generate display lists using
libultra. Since one of the goals of this project is to build a
library-independent abstraction layer, it is currently not a priority to
document the format of these files.

When exporting a level, only the name of each material is used. The actual
material information is looked up in the `.skm.yaml` files using the name. This
means that a level's appearance in Blender will not necessarily match the game.
However, for convenience, `assets/materials/materials.blend` contains Blender
versions of the various materials with the proper names so that it is easier to
apply them and also get a reasonable visual approximation while editing.

## TODO

* Asset pipeline
* Blender scripts
* Skeletool
* Exporter scripts
* Material YAML file structure (ideally after library-independent abstraction is done)
