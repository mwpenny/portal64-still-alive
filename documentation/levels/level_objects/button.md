# @button

A flat, circular button which emits a [signal](../signals.md) while pressed.

## Name Structure

```
@button PRESS_SIGNAL_NAME [OBJECT_PRESS_SIGNAL_NAME]
```

## Arguments

| Name                                  | Description                                                                                        |
| ------------------------------------- | -------------------------------------------------------------------------------------------------- |
| `PRESS_SIGNAL_NAME`                   | The name of the signal to emit while pressed                                                       |
| `OBJECT_PRESS_SIGNAL_NAME` (optional) | The name of the signal to emit while pressed by an object. Use `-1` or omit to indicate no signal. |

## Notes

The secondary "object press" signal is useful for only taking an action once a
level is properly solved (i.e., the button is pressed by a cube and not the
player's body).

The secondary signal will be emitted when any grabbable object with a mass over
the button's threshold of `1.9` is placed on it. Currently cubes and turrets are
the only objects that satisfy this criterion.
