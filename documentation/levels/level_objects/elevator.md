# @elevator

The start/end of level elevator that takes the player between test chambers.

## Name Structure

```
@elevator NAME DESTINATION
```

## Arguments

| Name                     | Description                                                                                                                                                                               |
| ------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `NAME`                   | The name of the elevator, used by other departure elevators to identify it as a destination                                                                                               |
| `DESTINATION` (optional) | Where departure elevators should send the player. Either the name of another elevator on the same map, or "next_level" to load the next map. Leave empty to indicate an arrival elevator. |

## Notes

Departure elevators open when the player is near and lock once they step inside.
Then, after some time, the screen shake animation and movement sound are played.
Finally, the player's portals are closed and they are teleported to the
destination elevator at the same relative position and rotation as in the
starting elevator to create the illusion that it is the same. It does not
actually move.

If a GLaDOS voice line is playing or queued, departure elevators into the next
map will not start their movement sequence until all lines finish.

Arrival elevators are much simpler. After a short delay, the doors open and the
chime sound is played. When the player steps away, the doors close and lock and
an autosave is created.
