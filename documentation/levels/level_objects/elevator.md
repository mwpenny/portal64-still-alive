# @elevator

The start/end of level elevator that takes the player between test chambers.

## Name structure

```
@elevator NAME DESTINATION
```

## Arguments

| Name                     | Description                                                                                                                                                                             |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `NAME`                   | The name of the elevator, used by other ending elevators to identify it as a destination                                                                                                |
| `DESTINATION` (optional) | Where ending elevators should send the player. Either the name of another elevator on the same map, or "next_level" to load the next map. Leave empty to indicate a starting elevator.  |

## Notes

End of level elevators open when the player is near and lock once they step
inside. Then, after some time, the screen shake animation and movement sound
are played. Finally, the player's portals are closed and they are teleported to
the destination elevator at the same relative position as in the starting
elevator to create the illusion that it is the same. It does not actually move.

If a GLaDOS voice line is playing, the last elevator in a map will not start to
transition the player to the next one until the line finishes.

Start of level elevators are much simpler. After a short delay, the doors open
and the chime sound is played. When the player steps away, the doors close and
lock and an autosave is created.
