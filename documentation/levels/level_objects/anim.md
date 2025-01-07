# @anim

An animatable bone of an armature.

## Name Structure

```
@anim ARMATURE_NAME [sound_type SOUND_TYPE_NAME]
```

## Arguments

| Name                                    | Description                                                                                                                              |
| --------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `ARMATURE_NAME`                         | The name of the armature the bone is a part of                                                                                           |
| `sound_type SOUND_TYPE_NAME` (optional) | If specified, `SOUND_TYPE_NAME` will play while the armature is moving. Only the sound type of the first bone that specifies it is used. |

## Notes

Bone animations are defined using standard Blender animations which, when
played, transform child [static geometry](./static.md) and
[dynamic collision](./dynamic_box.md) objects. This is useful for moving walls
and platforms. Only position and rotation can be animated.

Bones can be their own independent objects or part of a Blender armature, which
allows multiple to be transformed by the same animation. Bones must have
matching armature name arguments and not be nested in order to be animated
together. This is due to the way animations are serialized at level export time
and how they are played back.

Level animations are triggered in [cutscenes](../cutscenes/README.md) by
specifying an armature name and one of its animations to play. An armature can
be thought of as a single logical level element which is updated independently
of others.

If `player` is used as the armature name, playing an animation will additionally
take control away from the player and animate their view and body.

The possible values for `SOUND_TYPE_NAME` are:
* `LightRail`: Horizontal moving platform sound
* `Piston`: Vertical moving platform sound
* `Arm`: Horizontal moving wall sound
* `Stairs`: Ascending/descending staircase sound
* `Door`: Door open/close sound
