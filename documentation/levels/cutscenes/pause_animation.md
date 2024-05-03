# pause_animation

Pauses the current animation of a specified [armature](../level_objects/anim.md).

## Syntax

```
pause_animation ARMATURE_NAME
```

## Arguments

| Name               | Description                       |
| ------------------ | --------------------------------- |
| `ARMATURE_NAME`    | The name of the armature to pause |

## Notes

Pausing is equivalent to using the [play_animation](./play_animation.md)
cutscene step with a speed of 0 and the animation name set to the armature's
current animation.

This cutscene step has no effect if the specified armature has no active
animation.
