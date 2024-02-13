# @ball_launcher

A spawner for high energy pellets, which can be caught by a
[ball catcher](./ball_catcher.md) to emit a [signal](../signals.md).

## Name structure

```
@ball_launcher ACTIVE_SIGNAL_NAME [BALL_LIFETIME_SECONDS] [TARGET_BALL_VELOCITY]
```

## Arguments

| Name                               | Description                                                                                                                                         |
| ---------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ACTIVE_SIGNAL_NAME`               | The name of the signal which enables the launcher when set                                                                                          |
| `BALL_LIFETIME_SECONDS` (optional) | The amount of time in seconds it takes for a launched ball to destroy itself. Resets when passing through portals. Defaults to 10 if not specified. |
| `TARGET_BALL_VELOCITY`  (optional) | The intended velocity of launched balls. Any deviations (e.g., due to collision) will be corrected to this value. Defaults to 3.                    |

## Notes

While activated with its signal, a ball launcher ensures its associated ball has
been spawned - creating a new one if it doesn't exist. Launched balls have a
configurable lifetime until they are destroyed and the process is repeated. This
value constantly counts down and is reset when passing through portals.

Once a launcher's ball has been caught by a ball catcher, the launcher will not
spawn any more.
