# @box_dropper

A spawner for cube [decor](./decor.md) objects.

## Name Structure

```
@box_dropper ACTIVE_SIGNAL_NAME
```

## Arguments

| Name                 | Description                                               |
| -------------------- | --------------------------------------------------------- |
| `ACTIVE_SIGNAL_NAME` | The name of the signal which enables the dropper when set |

## Notes

When activated with its [signal](../signals.md) after being deactivated, if the
dropper previously spawned a cube that exists then it is destroyed.

As long as the dropper is active, it will dispense a new cube if it has not
already or if the previous one has been destroyed. When a cube is released, a
new one will fill the chamber to prepare for its next opening.

Box droppers use the `CUBE_UNIMPORTANT` decor type for their cubes so they will
not respawn automatically when destroyed and can instead be managed by the
dropper itself.
