# @doorway_cover

A mask which hides non-visible [doorways](./doorway.md), optionally controlling
their visibility gradually based on distance.

Useful for covering the out-of-bounds void when a [room](./room.md) is
explicitly hidden, or for culling in situations where the player can see a
doorway both close up and too far away to perceive detail.

## Name Structure

```
@doorway_cover [COLOR] [fade START_DISTANCE END_DISTANCE [AXIS]]
```

## Arguments

| Name                                                 | Description                                                                |
| ---------------------------------------------------- | ---------------------------------------------------------------------------|
| `COLOR` (optional)                                   | The hex representation of the cover's color. Defaults to `000000` (black). |
| `fade START_DISTANCE END_DISTANCE [AXIS]` (optional) | If specified, the cover's opacity is set based on the view distance's position between the given start and end values. At full opacity, the associated doorway is fully covered and considered non-visible. `AXIS` is an optional comma-separated 3D vector which, if specified, limits distance checks to that axis. |

## Notes

The cover applies to the first coplanar doorway found which contains its origin.

### Non-Visible Doorway Occlusion

If one of the doorway's rooms is not visible but the cover is, it is rendered
opaque. This can happen when a doorway is closed or a room's `can_see` argument
is used to hide unoccluded rooms (useful for reducing draw distance). Using a
doorway cover in such cases prevents seeing out of bounds. For example, the room
on the other side of a window could be made non-visible from far rooms and
obscured with a doorway cover to make the window appear opaque.

This strategy works best if the player's line of sight to the doorway is
interrupted between the near and far rooms so there is no abrupt transition.
It is helpful when distance-based culling is not required or cannot be used
(e.g., close viewing is tolerable from some rooms but not others).

### Distance-Based Culling

Distance-based culling is enabled if the `fade` argument is specified. It
controls doorway visibility itself, and is useful for those that stay in the
player's line of sight as they move closer or farther (culling with a room's
`can_see` argument would be too noticeable). Distance-based culling does not
override rooms explicitly marked as non-visible.

When viewed from a distance farther than the fade end distance, the cover is
rendered opaque and the doorway is excluded from room visibility checks (causing
the room behind it to not be drawn unless visible through other doorways).
Unlike doorways linked to closed [doors](./door.md), objects and raycasts can
still pass through.

When viewed from closer distances, the doorway is made visible and the cover is
rendered translucent with an opacity corresponding to the position between the
fade start and end distances. Distances below the fade start distance result in
the cover not being rendered at all (i.e., transparent).

To avoid jarring transitions, choose a color similar to dominant colors of the
room, and start and end distances such that fading does not occur until room
details are difficult to see.
