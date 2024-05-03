# clear_signal

Configures a signal to be unset by default.

## Syntax

```
set_signal SIGNAL_NAME
```

## Arguments

| Name          | Description                                           |
| ------------- | ----------------------------------------------------- |
| `SIGNAL_NAME` | The name of the signal to change the default state of |

## Notes

Signal-emitting sources such as level objects and operators work by inverting
the default state. That is, emitting a signal which is set by default will
result in it being read as unset.
