# wait_for_animation

Blocks until a specified [armature](../level_objects/anim.md) has no in-progress
animation.

## Syntax

```
wait_for_animation ARMATURE_NAME
```

## Arguments

| Name               | Description                          |
| ------------------ | ------------------------------------ |
| `ARMATURE_NAME`    | The name of the armature to wait for |

## Notes

The state of the armature's current animation (playing/paused) has no effect.
For this cutscene step to finish, the armature must have no current animation.
