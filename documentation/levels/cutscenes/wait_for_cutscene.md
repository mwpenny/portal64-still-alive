# wait_for_cutscene

Blocks until a specified cutscene is complete.

## Syntax

```
wait_for_cutscene CUTSCENE_NAME
```

## Arguments

| Name            | Description                          |
| --------------- | ------------------------------------ |
| `CUTSCENE_NAME` | The name of the cutscene to wait for |

## Notes

If multiple instances of the cutscene are running, this step will block until
all have ended.
