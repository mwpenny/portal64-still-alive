# @doorway_cover

A mask which gradually fades in/out and controls [doorways](./doorway.md) based
on distance. Useful for culling in situations where the player can still see a
doorway but is too far away to perceive detail.

## Name Structure

```
@doorway_cover FADE_START_DISTANCE FADE_END_DISTANCE [COLOR]
```

## Arguments

| Name                     | Description                                                                   |
| ------------------------ | ----------------------------------------------------------------------------- |
| `FADE_START_DISTANCE`    | The view distance at which the doorway begins to be covered.                  |
| `FADE_END_DISTANCE`      | The view distance at which the doorway becomes fully covered and closes.      |
| `COLOR` (optional)       | The hex representation of the color to fade in. Defaults to `ffffff` (black). |

## Notes

The cover controls the first coplanar doorway found which contains its origin.

When viewed from a distance farther than the fade end distance, the cover is
rendered opaque and the doorway is closed (causing the room behind it to not be
drawn). When viewed from closer distances, the doorway is opened and the cover
is rendered translucent with an opacity corresponding to the position between
the fade start and end distances. Distances below the fade start distance result
in the cover not being rendered at all (i.e., transparent).

To avoid jarring transitions, choose a color similar to dominant colors of the
room, and start and end distances such that fading does not occur until room
details are difficult to see.
