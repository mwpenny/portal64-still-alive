# teleport_player

Moves the player to a specified [location](../level_objects/location.md).

## Syntax

```
teleport_player SOURCE_LOCATION_NAME TARGET_LOCATION_NAME
```

## Arguments

| Name                   | Description                                                                                                                        |
| ---------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `SOURCE_LOCATION_NAME` | The location the player is being teleported from. Used to calculate relative position and rotation for the destination.            |
| `TARGET_LOCATION_NAME` | The location to teleport the player to. They are positioned with the same relative position and rotation they had from the source. |
