# open_portal

Opens a specified portal at a given [location](../level_objects/location.md).

## Syntax

```
open_portal LOCATION_NAME [PORTAL_INDEX] [pedestal]
```

## Arguments

| Name                      | Description                                                                                                       |
| ------------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `LOCATION_NAME`           | The name of the location level object to position the portal at                                                   |
| `PORTAL_INDEX` (optional) | The portal to open (`0` for orange, `1` for blue). Defaults to orange.                                            |
| `pedestal` (optional)     | If specified, the portal will be fired from the first [pedestal](../level_objects/pedestal.md) found in the level |
