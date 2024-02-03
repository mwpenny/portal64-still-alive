# @doorway

A connection between two [room](./room.md)s. Used for broad phase collision and
culling purposes, to determine room visibility and movement of dynamic objects
from one room to another.

## Name structure

```
@doorway
```

## Notes

Doorways connect their nearest two rooms, and can be open or closed (i.e., when
associated with a [door](./door.md)). If a doorway is closed, its rooms are not
considered connected at that point (other open doorways count).

Doorways do not need to be linked to a physical door in the world. For example,
a doorway can be placed over a window so that the room on the other side is
not processed when out of the player's view.
