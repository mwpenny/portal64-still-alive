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

Due to the way they are rendered (by dynamically modifying the model and its
texture coordinates), only one unique sign can be drawn - and therefore on
screen - at a time. Placing more than one will cause them both to appear as
whichever was processed last.
