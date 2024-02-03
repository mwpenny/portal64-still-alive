# @collision

Static level collision geometry. Must be a quad.

## Name structure

```
@collision [transparent] [thickness depth] [CL_X...]
```

## Arguments

| Name                       | Description                                                                              |
| -------------------------- | ---------------------------------------------------------------------------------------- |
| `transparent` (optional)   | If `transparent` is specified, the `TRANSPARENT` collision layer will be added           |
| `thickness num` (optional) | If `thickness` is specified, the quad's thickness will be set to `depth`                 |
| `CL_X` (optional)          | Space-separated list of colllision layer names. Only those prefixed with `CL_` are used. |

## Notes

Game objects in the same [room](./room.md) and on the same collision layer can
collide. Some layers are also used for certain checks by the game.

If no collision layers are specified, the defaults are `STATIC`, `TANGIBLE`, and
`BLOCK_BALL`. The possible layers and their uses are as follows.

* `STATIC`: Used for portal raycasts, and by:
    * Doors
    * Elevators
* `TRANSPARENT`: Unused
* `TANGIBLE`: Used to check grabbing and walking, and by:
    * Ball catchers
    * Ball launchers
    * [Button](./button.md)s
    * Decor
    * [Door](./door.md)s (when open)
    * Dynamic boxes
    * Elevators
    * Player
    * Portals
    * Security cameras
    * Switches
    * Triggers
* `GRABBABLE`: Used to check grabbing, and by:
    * Decor
    * Security cameras (when detached)
* `FIZZLER`: Used to block portal raycasts, and by:
    * Decor
    * Fizzlers
    * Player
    * Security cameras
* `BLOCK_PORTAL`: Used for portal raycasts, and by:
    * Fizzlers
* `BLOCK_BALL`: Used by:
    * Energy balls
    * Ball launchers
    * Dynamic boxes
    * Elevators
    * Player

## TODO

* Page on collision detection
* Link to level object pages when created
