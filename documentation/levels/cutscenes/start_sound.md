# start_sound

Starts a sound immediately without blocking.

## Syntax

```
start_sound SOUND_ID [VOLUME] [SPEED]
```

## Arguments

| Name                | Description                                                                                                          |
| ------------------- | -------------------------------------------------------------------------------------------------------------------- |
| `SOUND_ID`          | The ID of the sound to start. `SOUNDS_` prefix can be omitted.                                                       |
| `VOLUME` (optional) | The volume multiplier for playback. 0 is muted and 1 is normal volume. Defaults to 1.                                |
| `SPEED` (optional)  | The speed to play the sound at. Effectively a multiplier of the game's output sample rate (22050 Hz). Defaults to 1. |

## Notes

Only 12 sounds can be playing simultaneously. Any sounds started while at the
limit are treated as if they have a length of zero.
