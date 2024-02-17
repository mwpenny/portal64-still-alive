# @decor

A generic dynamic object without its own dedicated logic.

## Name structure

```
@decor TYPE
```

## Arguments

| Name   | Description                                                    |
| ------ | -------------------------------------------------------------- |
| `TYPE` | The type of decor. Used to determine its model and properties. |

## Notes

Decor objects are useful since they allow simple, pre-defined object
configurations to be easily reused without the need for object-specific code.

If a decor object has collision then it is grabbable, fizzlable, and affected by
physics. Fizzlable objects can be destroyed when they touch
[fizzlers](./fizzler.md) or fall out of bounds.

Decor objects can play a sound, which emits from their 3D position in the world
and attenuates with distance.

All properties of decor objects are determined using their type. Valid decor
type names are:

| Type                | Description                                                                          |
| ------------------- | ------------------------------------------------------------------------------------ |
| `CYLINDER`          | Test object. Unused. Has collision.                                                  |
| `RADIO`             | Plays "Still Alive" jazz arrangement. Has collision.                                 |
| `CUBE`              | A weighted storage cube. Has collision. Respawns when destroyed.                     |
| `CUBE_UNIMPORTANT`  | A weighted storage cube. Has collision. Does not respawn when destroyed.             |
| `AUTOPORTAL_FRAME`  | Border denoting [location](./location.md) where scripted portals open. No collision. |
| `LIGHT_RAIL_ENDCAP` | Moving platform light beam base. No collision.                                       |
| `LAB_MONITOR`       | Observation room computer monitor. No collision.                                     |
| `LAB_CHAIR`         | Observation room chair. No collision.                                                |
| `LAB_DESK01`        | Observation room desk variant 1. No collision.                                       |
| `LAB_DESK02`        | Observation room desk variant 2. No collision.                                       |
| `LAB_DESK03`        | Observation room desk variant 3. No collision.                                       |
| `LAB_DESK04`        | Observation room desk variant 4. No collision.                                       |
