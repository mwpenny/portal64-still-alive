# resume_animation

Updates the speed of a specified [armature](../level_objects/anim.md)'s current
animation.

## Syntax

```
resume_animation ARMATURE_NAME
```

## Arguments

| Name               | Description                                                                                                            |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------- |
| `ARMATURE_NAME`    | The name of the armature to animate                                                                                    |
| `SPEED` (optional) | The speed multiplier for playback. 1 is normal speed and negative speeds play the animation in reverse. Defaults to 1. |

## Notes

Resuming is equivalent to using the [play_animation](./play_animation.md)
cutscene step with the animation name set to the armature's current animation.

This cutscene step has no effect if the specified armature has no active
animation.
