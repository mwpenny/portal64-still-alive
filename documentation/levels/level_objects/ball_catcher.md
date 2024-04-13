# @ball_catcher

A receptacle for a high energy pellet launched by a
[ball launcher](./ball_launcher.md). Emits a [signal](../signals.md) when a
ball is caught.

## Name structure

```
@ball_catcher CATCH_SIGNAL_NAME
```

## Arguments

| Name                | Description                                          |
| ------------------- | ---------------------------------------------------- |
| `CATCH_SIGNAL_NAME` | The name of the signal to emit when a ball is caught |

## Notes

When a ball collides with a ball catcher, the catcher notifies the associated
launcher (so it will not update the ball) and emits the specified signal.

Only one ball can be caught.
