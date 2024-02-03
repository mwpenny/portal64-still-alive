# @door

A sliding door which can open or close in response to a [signal](../signals.md).

## Name structure

```
@door open_signal_name [type]
```

## Arguments

| Name                     | Description                                                                        |
| -------------------------| ---------------------------------------------------------------------------------- |
| `open_signal_name`       | The name of the signal to which will open the door when set                        |
| `type` (optional)        | The type of door. `02` for vertical sliding hatches, otherwise circular exit door. |

## Notes

At level export time, if a [doorway](./doorway.md) is found on a door (coplanar
and at the same position on that plane), the two are linked and the doorway will
open and close with the door.
