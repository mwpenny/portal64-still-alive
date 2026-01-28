# @signage

The large, illuminated informational sign at the beginning of each test chamber.

## Name Structure

```
@signage TEST_CHAMBER_NUMBER
```

## Arguments

| Name                  | Description                                                                                     |
| --------------------- | ----------------------------------------------------------------------------------------------- |
| `TEST_CHAMBER_NUMBER` | The number of the test chamber to display on the sign, along with its associated warning images |

## Notes

Signage is unlit by default. It can be activated using the
[activate_signage](../cutscenes/activate_signage.md)
[cutscene](../cutscenes/README.md) step.

Due to the way they are rendered (by dynamically modifying the texture
coordinates of a shared model), only one unique sign can be shown at a time.
Placing more than one will cause them all to appear as whichever was processed
last.
