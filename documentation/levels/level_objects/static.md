# @static

Static level geometry. Used to generate level mesh and portal surface data.

## Name Structure

```
@static [no_portals|accept_portals] [indicator_lights SIGNAL_NAME] [uvtransx TRANS_X] [uvtransy TRANS_Y] [uvtransz TRANS_Z] [uvrotx ROT_X] [uvroty ROT_Y] [uvrotz ROT_Z] [uvscale SCALE] [precise_culling]
```

## Arguments

| Name                                      | Description                                                                                                                                                                      |
| ----------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `no_portals` (optional)                   | If specified, the surface will not be considered portalable, regardless of its material name. This argument cannot be used at the same time as `accept_portals`.                 |
| `accept_portals` (optional)               | If specified, the surface will be considered portalable, regardless of its material name. This argument cannot be used at the same time as `no_portals`.                         |
| `indicator_lights SIGNAL_NAME` (optional) | If specified and the indicator light or door state material is used, it will be switched to the corresponding "active" variant when the specified [signal](../signals.md) is set |
| `uvtransx TRANS_X` (optional)             | If specified and the material has `tileSizeS` and `tileSizeT` properties, translates UV coordinates by `TRANS_X` on the X axis. Defaults to 0.                                   |
| `uvtransy TRANS_Y` (optional)             | If specified and the material has `tileSizeS` and `tileSizeT` properties, translates UV coordinates by `TRANS_Y` on the Y axis. Defaults to 0.                                   |
| `uvtransz TRANS_Z` (optional)             | If specified and the material has `tileSizeS` and `tileSizeT` properties, translates UV coordinates by `TRANS_Z` on the Z axis. Defaults to 0.                                   |
| `uvrotx ROT_X` (optional)                 | If specified and the material has `tileSizeS` and `tileSizeT` properties, rotates UV coordinates by `ROT_X` degrees on the X axis. Defaults to 0.                                |
| `uvroty ROT_Y` (optional)                 | If specified and the material has `tileSizeS` and `tileSizeT` properties, rotates UV coordinates by `ROT_Y` degrees on the Y axis. Defaults to 0.                                |
| `uvrotz ROT_Z` (optional)                 | If specified and the material has `tileSizeS` and `tileSizeT` properties, rotates UV coordinates by `ROT_Z` degrees on the Z axis. Defaults to 0.                                |
| `uvscale SCALE` (optional)                | If specified and the material has `tileSizeS` and `tileSizeT` properties, scales UV coordinates by `SCALE/tileSize` in each direction. Defaults to 1.                            |
| `precise_culling` (optional)              | If specified, the object's exact bounding box will be used for culling. This requires extra work but prevents geometry behind portals from showing through.                      |

## Notes

Beyond the arguments listed above, static geometry is used for its vertex
coodrinates, vertex colors, material information, and UV coordinates. It is not
used for collision. Instead there are dedicated [static](./collision.md) and
[dynamic](./dynamic_box.md) collision objects.

With the exception of [ambient](./ambient.md) and [point](./point_light.md)
light bake types, materials specified in Blender are only used for their
name, which serves as a key to look up their actual data elsewhere
(see [Level File Formats](../file_formats.md#materials)) and to denote which
surfaces are portalable. The following material names allow portals to be placed:

* `concrete_bts_modular_wall001c`
* `concrete_modular_ceiling001a`
* `concrete_modular_floor001a`
* `concrete_modular_wall001b`
* `concrete_modular_wall001d`
* `transparent_portal_surface`

This behavior can be overridden by using the `accept_portals` or `no_portals`
arguments. Only one can be specified at a time. To accept portals, the geometry
must also have associated collision so that it can be hit. This can be done by
placing a coplanar static collision object or through animation with a
[bone](./anim.md) that also animates a dynamic collision object.

A mesh's UV coordinates are only used if its material does not have the
`tileSizeS` and `tileSizeT` properties in its YAML definition. Otherwise, the
`uvtransx`/`uvtransy`/`uvtransz`, `uvrotx`/`uvroty`/`uvrotz`, and `uvscale`
arguments can be specified to manipulate UV maps.
