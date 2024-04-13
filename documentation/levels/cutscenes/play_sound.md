# play_sound

Plays a sound immediately and blocks until it has finished (unless looped).

## Syntax

```
play_sound SOUND_ID [VOLUME] [SPEED]
```

## Arguments

| Name                | Description                                                                                                          |
| ------------------- | -------------------------------------------------------------------------------------------------------------------- |
| `SOUND_ID`          | The ID of the sound to play. `SOUNDS_` prefix can be omitted.                                                        |
| `VOLUME` (optional) | The volume multiplier for playback. 0 is muted and 1 is normal volume. Defaults to 1.                                |
| `SPEED` (optional)  | The speed to play the sound at. Effectively a multiplier of the game's output sample rate (44100 Hz). Defaults to 1. |

## Notes

Only 12 sounds can be playing simultaneously. Any sounds played while at the
limit are treated as if they have a length of zero.

The game outputs audio at 44100 Hz but most sounds are sampled at 22050 Hz, and
so a speed of 0.5 is needed for normal playback in the majority of cases.
