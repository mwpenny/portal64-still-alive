# @clock

A simple countdown clock, as seen in Test Chamber 00.

## Name Structure

```
@clock DURATION_SECONDS
```

## Arguments

| Name               | Description                                     |
| ------------------ | ----------------------------------------------- |
| `DURATION_SECONDS` | The amount of time in seconds to count down for |

## Notes

While the amount of time the clock has been running is less than
`DURATION_SECONDS`, its display digits will be updated.

After the specified duration has elapsed, the clock will remain at `00:00:00:00`.

Due to the way they are rendered (by dynamically modifying the texture
coordinates of a shared model), only one unique clock can be shown at a time.
Placing more than one will cause them all to appear as whichever was processed
last.
