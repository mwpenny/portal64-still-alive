# @room

A box which groups objects in a level. Used for broad phase collision and
culling.

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
know their room index.

For performance reasons, only objects in the current room and rooms connected
via open [doorway](./doorway.md)s are considered for rendering and collision
detection.
