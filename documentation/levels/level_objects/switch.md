# @switch

A button on a cylindrical pedestal which, on press, emits a
[signal](../signals.md) until a specified period of time has elapsed.

## Name structure

```
@switch press_signal_name duration_seconds
```

## Arguments

| Name                | Description                                                                 |
| --------------------| --------------------------------------------------------------------------- |
| `press_signal_name` | The name of the signal to emit while counting down after a press            |
| `duration_seconds`  | The amount of time in seconds to emit `press_signal_name` for after a press |

## Notes

A tick-tock sound will be played while the signal is being emitted.

Pressing the switch again before the timeout has elapsed will not reset the timer.
