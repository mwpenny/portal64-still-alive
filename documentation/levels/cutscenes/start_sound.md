# start_sound

Starts a sound immediately without blocking.

## Syntax

```
start_sound SOUND_ID [VOLUME] [SPEED]
```

## Arguments

| Name                | Description                                                                                           |
| ------------------- | ----------------------------------------------------------------------------------------------------- |
| `SOUND_ID`          | The ID of the sound to start. `SOUNDS_` prefix can be omitted.                                        |
| `VOLUME` (optional) | The volume to play the sound at. 0 is muted and 1 is normal volume.                                   |
| `SPEED` (optional)  | The speed to play the sound at. Effectively a multiplier of the game's output sample rate (44100 Hz). |

## Notes

Only 12 sounds can be playing simultaneously. Any sounds started while at the
limit are treated as if they have a length of zero.

The game outputs audio at 44100 Hz but most sounds are sampled at 22050 Hz, and
so a speed of 0.5 is needed for normal playback in the majority of cases.
