# @door

A sliding door which can open or close in response to a [signal](../signals.md).

## Name Structure

```
@door OPEN_SIGNAL_NAME [TYPE]
```

## Arguments

| Name                     | Description                                                                        |
| ------------------------ | ---------------------------------------------------------------------------------- |
| `OPEN_SIGNAL_NAME`       | The name of the signal which will open the door when set                           |
| `TYPE` (optional)        | The type of door. `02` for vertical sliding hatches, otherwise circular exit door. |

## Notes

At level export time, if a [doorway](./doorway.md) contains a door (coplanar and
contains the door's origin), the two are linked and the game will open and close
the doorway with the door. In this case, the door is considered to exist inside
both of its connected [rooms](./room.md).
