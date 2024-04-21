# wait_for_signal

Blocks until a specified signal is set.

## Syntax

```
wait_for_signal SIGNAL_NAME [REQUIRED_FRAMES]
```

## Arguments

| Name                         | Description                                                                                                               |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `SIGNAL_NAME`                | The name of the signal to wait for                                                                                        |
| `REQUIRED_FRAMES` (optional) | The number of frames the signal must have been previously set for. Useful for debouncing or adding delays. Defaults to 0. |

## Notes

If using the `REQUIRED_FRAMES` argument, note that cutscenes are updated at 30
FPS on NTSC/PAL-M consoles and 25 FPS on PAL consoles.
