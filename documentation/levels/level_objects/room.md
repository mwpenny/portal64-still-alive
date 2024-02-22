# @room

A 3D box which defines a portion of a level. Used for culling and broad phase
collision.

## Name structure

```
@room INDEX
```

## Arguments

| Name    | Description                                  |
| ------- | -------------------------------------------- |
| `INDEX` | The index number of the room the box defines |

## Notes

At level export time, room regions with the same index are merged to define the
bounds of one logical room. Game objects within a room (including the player)
know their room index, which is updated when passing through
[doorways](./doorway.md).

For performance reasons, only the player's current room and those visible
through open doorways are considered for rendering, and only objects in the same
room can collide with each other.
