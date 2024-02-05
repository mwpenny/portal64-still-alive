# @button

A flat, circular button which emits a [signal](../signals.md) while pressed.

## Name structure

```
@button PRESS_SIGNAL_NAME OBJECT_PRESS_SIGNAL_NAME
```

## Arguments

| Name                       | Description                                               |
| -------------------------- | --------------------------------------------------------- |
| `PRESS_SIGNAL_NAME`        | The name of the signal to emit while pressed              |
| `OBJECT_PRESS_SIGNAL_NAME` | The name of the signal to emit while pressed by an object |

## Notes

The secondary "object press" signal is useful for only taking an action once a
level is properly solved, such as when the button is pressed by a cube and not
the player's body.

Although intended for cubes, the secondary signal will be emitted when any
grabbable object over the button's threshold of `1.9` is placed on it. Currently
cubes are the only objects that satisfy this criterion with a mass of `2.0`.
