# @ambient

A 3D box whose vertex colors allow ambient lighting to be calculated for
[static geometry](./static.md).

## Name Structure

```
@ambient
```

## Notes

Ambient light sources are not used at runtime. Rather, they are used by the
[tools/blender/bake_lighting.py](../../../tools/blender/bake_lighting.py)
Blender script to precompute vertex colors for static geometry. This script
must be run manually in Blender prior to exporting a level.

Baked colors are chosen for a given vertex by considering the (up to) two
closest ambient light sources. The color contribution of each found source is
calculated by interpolating between its vertex colors based on the target's
position. Finally, the contributions are blended based on each source's distance
from the target. Either smooth or flat shading can be used.

In order for an object to receive baked lighting, it must have a mesh whose
material has the custom property `bakeType` set to `lit` in Blender. The
materials in `assets/materials/materials.blend` are tagged appropriately.

Instead of using this approach, it is also possible to manually assign vertex
colors to static geometry using the `Col` color attribute layer. Ambient light
sources just make it easier to simulate and edit lighting.
