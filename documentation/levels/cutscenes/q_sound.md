# q_sound

Queues a sound to play without blocking.

## Syntax

```
q_sound SOUND_ID CHANNEL_NAME SUBTITLE_ID [VOLUME]
```

## Arguments

| Name                | Description                                                                                                                                                                          |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `SOUND_ID`          | The ID of the sound to queue. `SOUNDS_` prefix can be omitted.                                                                                                                       |
| `CHANNEL_NAME`      | The name of the channel to queue the sound on                                                                                                                                        |
| `SUBTITLE_ID` (optional) | The ID of the string to display as a subtitle when playing the sound with subtitles enabled, or `StringIdNone`. Only shown for `CH_GLADOS` channel. Defaults to `StringIdNone`. |
| `VOLUME` (optional) | The volume multiplier for playback. 0 is muted and 1 is normal volume. Defaults to 1.                                                                                                |

## Notes

There are three cutscene sound channels:

* `CH_GLADOS`: Intended for GLaDOS dialog. The intercom chime sound is inserted
               automatically, subtitles are shown (if enabled), and other sounds
               are dampened while this channel is playing.
* `CH_MUSIC`: Intended for music. Affected by the music volume setting in the
              audio options menu.
* `CH_AMBIENT`: Intended for test chamber ambience.

Only one sound per channel is played at a time. A channel's current sound is
played in its entirety before moving on to the next one queued on a first in,
first out basis.

A maximum of 25 sounds can be queued for each channel. Any queued while at the
limit will be ignored.
