# @door

A sliding door which can open or close in response to a [signal](../signals.md).

## Name structure

```
@door OPEN_SIGNAL_NAME [TYPE]
```

## Arguments

| Name                     | Description                                                                        |
| -------------------------| ---------------------------------------------------------------------------------- |
| `OPEN_SIGNAL_NAME`       | The name of the signal to which will open the door when set                        |
| `TYPE` (optional)        | The type of door. `02` for vertical sliding hatches, otherwise circular exit door. |

## Notes

At level export time, if a [doorway](./doorway.md) is found on a door (coplanar
and at the same position on that plane), the two are linked and the doorway will
open and close with the door.
