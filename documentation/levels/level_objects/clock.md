# @clock

A simple countdown clock, as seen in Test Chamber 00.

## Name structure

```
@clock duration_seconds
```

## Arguments

| Name               | Description                                     |
| ------------------ | ----------------------------------------------- |
| `duration_seconds` | The amount of time in seconds to count down for |

## Notes

While the amount of time the clock has been running is less than
`duration_seconds`, its display digits will be updated.

After the specified duration has elapsed, the clock will remain at `00:00:00:00`.
