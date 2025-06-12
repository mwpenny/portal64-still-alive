# @turret

A sentry turret enemy.

## Name Structure

```
@turret [player_can_autotip]
```

## Arguments

| Name                            | Description                                                                             |
| ------------------------------- | --------------------------------------------------------------------------------------- |
| `player_can_autotip` (optional) | If specified, collisions with the player will have their horizontal movement amplified. |

## Notes

The `player_can_autotip` argument allows the player to easily knock over turrets
via the same amplification that is used for physics objects (e.g., cubes dropped
on top). This is useful for turrets blocking the player's path and/or intended
as a demonstration rather than an obstacle. For example, the first turret of
chamber 16. This applies until the turret leaves the idle state (grabbed, player
detected, etc.).
