# @room

A 3D box which defines a portion of a level. Used for culling and collision.

## Name Structure

```
@room INDEX [can_see R1,R2,...,RN]
```

## Arguments

| Name                              | Description                                                                                                                                                                                      |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `INDEX`                           | The index number of the room the box defines                                                                                                                                                     |
| `can_see R1,R2,...,RN` (optional) | If specified, rooms outside the comma-separated list will not be considered visible from the current room. This is useful for room shapes which cannot be handled by standard visibility checks. |

## Notes

At level export time, room regions with the same index are merged to define the
bounds of one logical room. Game objects within a room (including the player)
know their room index, which is updated when passing through
[doorways](./doorway.md).

For performance reasons, only the player's current room and those visible
through open doorways are considered for rendering, and objects can only collide
with static [collision](./collision.md) in their current room.

Levels are limited to 64 rooms.
