# @fizzler

An emancipation grill. When passing through, destroys [decor](./decor.md)
objects and any player-placed portals.

## Name Structure

```
@fizzler [CUBE_SIGNAL_NAME]
```

## Arguments

| Name                          | Description                                                                                    |
| ----------------------------- | ---------------------------------------------------------------------------------------------- |
| `CUBE_SIGNAL_NAME` (optional) | The name of the signal to emit when a cube is fizzled. Use `-1` or omit to indicate no signal. |

## Notes

The cube signal is only emitted on the frame that fizzling starts.
