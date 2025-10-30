# play_animation

Starts or updates a specified animation on a specified
[armature](../level_objects/anim.md).

## Syntax

```
play_animation ARMATURE_NAME ANIMATION_NAME [SPEED] [FLAGS]
```

## Arguments

| Name               | Description                                                                                                            |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------- |
| `ARMATURE_NAME`    | The name of the armature to animate                                                                                    |
| `ANIMATION_NAME`   | The name of the animation to play on the armature                                                                      |
| `SPEED` (optional) | The speed multiplier for playback. 1 is normal speed and negative speeds play the animation in reverse. Defaults to 1. |
| `FLAGS` (optional) | Comma-separated list of flags to apply to the animation.                                                               |

## Notes

Only one animation can be played at a time on a single armature. Playing a new
one will stop any in-progress animation. Playing an animation that is already
playing will update its speed. A speed of 0 will pause the animation.

If an armature named `player` is used, control will be taken away from the
player and their view and body will be animated in addition to the armature.

The possible flags are:
* `Loop`: Automatically repeat the animation once it is complete
