# point_pedestal

Rotates [pedestal](../level_objects/pedestal.md) objects toward a specified
[location](../level_objects/pedestal.md).

## Syntax

```
point_pedestal LOCATION_NAME [no_shooting_sound]
```

## Arguments

| Name                           | Description                                                                                 |
| ------------------------------ | ------------------------------------------------------------------------------------------- |
| `LOCATION_NAME`                | The name of the location to rotate toward                                                   |
| `no_shooting_sound` (optional) | If specified, the portal gun charging sound will not be played after the rotation completes |

## Notes

If multiple pedestals exist in the level, all are rotated.

Pedestal rotation only occurs horizontally.

By default, the portal gun charging sound is played after the rotation
completes, for a seamless transition between this cutscene step and
[open_portal](./open_portal.md).
