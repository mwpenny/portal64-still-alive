# @box_dropper

A spawner for cube [decor](./decor.md) objects.

## Name Structure

```
@box_dropper ACTIVE_SIGNAL_NAME [CUBE_TYPE]
```

## Arguments

| Name                   | Description                                               |
| ---------------------- | --------------------------------------------------------- |
| `ACTIVE_SIGNAL_NAME`   | The name of the signal which enables the dropper when set |
| `CUBE_TYPE` (optional) | The dropper's contents. Defaults to `Standard`.           |

## Notes

The possible values for `CUBE_TYPE` are:
* `None`: The dropper will contain no cubes and only open/close
* `Standard`: The dropper will dispense standard cubes

When its [signal](../signals.md) transitions from unset to set, the dropper
will open. If it previously spawned a cube that exists, the cube will be
destroyed first.

As long as the dropper is active and its cube type is not `None`, it will
dispense a new cube if it has not already or if the previous one has been
destroyed. When a cube is released, a new one will fill the chamber to prepare
for its next opening.

Box droppers use the `CUBE_UNIMPORTANT` decor type for their cubes so they will
not respawn automatically when destroyed and can instead be managed by the
dropper itself.
