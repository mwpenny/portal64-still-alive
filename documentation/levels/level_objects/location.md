# @location

A point in the world, used for level spawn points and can be referenced in
cutscenes.

## Name Structure

```
@location NAME
```

## Arguments

| Name   | Description              |
| ------ | ------------------------ |
| `NAME` | The name of the location |

## Notes

Locations can be referenced in cutscenes. See [Cutscenes](../cutscenes/README.md)
for more information.

Each level must have a location named `start`. This is used to place the player
when loading levels without a transition from a previous one (i.e., from a menu).
