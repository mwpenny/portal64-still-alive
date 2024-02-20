# @point_light

A point in space which allows omnidirectional light to be calculated for
[static geometry](./static.md).


## Name structure

```
@point_light
```

## Notes

Point light sources are not used at runtime. Rather, they are used by the
[tools/bake_lighting.py](../../../tools/bake_lighting.py) Blender script to
precompute vertex colors for static geometry. This script must be run manually
in Blender prior to exporting a level.

Each point light must be defined as a
[light object](https://docs.blender.org/manual/en/latest/render/lights/light_object.html#)
in Blender. When baking, the effect on a given vertex is calculated based on the
light's color, distance, angle to the target, and energy. Vertices are only
affected by a point light source if they are within range of its closest
[ambient light source](./ambient.md). Either smooth or flat shading can be used.

In order for an object to receive baked lighting, it must have a mesh whose
material has the custom property `bakeType` set to `lit` in Blender. The
materials in `assets/materials/materials.blend` are tagged appropriately.

Instead of using this approach, it is also possible to manually assign vertex
colors to static geometry using the `Col` color attribute layer. Point light
sources just make it easier to simulate and edit lighting.
