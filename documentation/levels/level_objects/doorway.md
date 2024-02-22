# @doorway

A connection between two [rooms](./room.md). Used for culling and broad phase
collision purposes, to determine room visibility and movement of dynamic objects
from one room to another.

## Name structure

```
@doorway
```

## Notes

Doorways connect their nearest two rooms, and can be open or closed (i.e., when
associated with a [door](./door.md)). If a doorway is closed, its rooms are not
considered connected at that point (other open doorways count). Only the
player's current room and visible connected rooms are considered for rendering.

Passing through an open doorway updates an object's room index, allowing it to
collide with other objects in the entered room while ignoring those in the
exited room.

Doorways do not need to be linked to a physical door in the world. For example,
a doorway can be placed over a window so that the room on the other side is
not considered for rendering when out of the player's view.
