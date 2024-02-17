# @collision

Static level collision geometry. Must be a quad.

## Name structure

```
@collision [transparent] [thickness DEPTH] [CL_X...]
```

## Arguments

| Name                         | Description                                                                              |
| ---------------------------- | ---------------------------------------------------------------------------------------- |
| `transparent` (optional)     | If specified, the `TRANSPARENT` collision layer will be added                            |
| `thickness DEPTH` (optional) | If specified, the quad's thickness will be set to `depth`                                |
| `CL_X` (optional)            | Space-separated list of colllision layer names. Only those prefixed with `CL_` are used. |

## Notes

Game objects in the same [room](./room.md) and on the same collision layer can
collide. Some layers are also used for certain checks by the game.

If no collision layers are specified, the defaults are `STATIC`, `TANGIBLE`, and
`BLOCK_BALL`. The possible layers and their uses are as follows.

* `STATIC`: Used for portal raycasts, and by:
    * [Doors](./door.md)
    * [Elevators](./elevator.md)
* `TRANSPARENT`: Unused
* `TANGIBLE`: Used to check grabbing and walking, and by:
    * [Ball catchers](./ball_catcher.md)
    * [Ball launchers](./ball_launcher.md)
    * [Buttons](./button.md)
    * [Decor](./decor.md) (with collision)
    * [Doors](./door.md) (when open)
    * [Dynamic boxes](./dynamic_box.md)
    * [Elevators](./elevator.md)
    * Player
    * Portals
    * [Security cameras](./security_camera.md)
    * [Switches](./switch.md)
    * [Triggers](./trigger.md)
* `GRABBABLE`: Used to check grabbing, and by:
    * [Decor](./decor.md) (with collision)
    * [Security cameras](./security_camera.md) (when detached)
* `FIZZLER`: Used to block portal raycasts, and by:
    * [Decor](./decor.md) (with collision)
    * [Fizzlers](./fizzler.md)
    * Player
    * [Security cameras](./security_camera.md)
* `BLOCK_PORTAL`: Used for portal raycasts, and by:
    * [Fizzlers](./fizzler.md)
* `BLOCK_BALL`: Used by:
    * Energy balls
    * [Ball launchers](./ball_launcher.md)
    * [Dynamic boxes](./dynamic_box.md)
    * [Elevators](./elevator.md)
    * Player

## TODO

* Page on collision detection
