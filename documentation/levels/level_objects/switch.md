# @switch

A button on a cylindrical pedestal which, on press, emits a
[signal](../signals.md) until a specified period of time has elapsed.

## Name structure

```
@switch PRESS_SIGNAL_NAME DURATION_SECONDS
```

## Arguments

| Name                | Description                                                                 |
| --------------------| --------------------------------------------------------------------------- |
| `PRESS_SIGNAL_NAME` | The name of the signal to emit while counting down after a press            |
| `DURATION_SECONDS`  | The amount of time in seconds to emit `PRESS_SIGNAL_NAME` for after a press |

## Notes

A tick-tock sound will be played while the signal is being emitted.

Pressing the switch again before the timeout has elapsed will not reset the timer.
