# @point_light

A point in space which allows omnidirectional light to be calculated for
[static geometry](./static.md).


## Name Structure

```
@point_light
```

## Notes

Point light sources are not used at runtime. Rather, they are used by the
[tools/blender/bake_lighting.py](../../../tools/blender/bake_lighting.py)
Blender script to precompute vertex colors for static geometry. This script
must be run manually in Blender prior to exporting a level. After running once,
it can be re-run directly or from `View > Vertex Bake Lighting`.

Each point light must be defined as a
[light object](https://docs.blender.org/manual/en/latest/render/lights/light_object.html#)
in Blender. When baking, the effect on a given vertex is calculated based on the
light's color, distance, angle to the target, and energy. Vertices are only
affected by a point light source if they are within range of its closest
[ambient light source](./ambient.md). Either smooth or flat shading can be used.

In order for geometry to receive baked lighting, it must not have the `nobake`
argument in its name and its Blender material must have the `bakeType` custom
property set to `lit`. The materials in `assets/materials/materials.blend` are
tagged appropriately.

Instead of using this approach, it is also possible to manually assign vertex
colors to static geometry using the `Col` color attribute layer. Point light
sources just make it easier to simulate and edit lighting.
