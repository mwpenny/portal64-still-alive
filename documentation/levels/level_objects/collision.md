# @collision

Static level collision geometry. Must be a quad.

## Name Structure

```
@collision [transparent] [thickness DEPTH] [CL_X]...
```

## Arguments

| Name                         | Description                                                                                                     |
| ---------------------------- | --------------------------------------------------------------------------------------------------------------- |
| `transparent` (optional)     | If specified, the `TRANSPARENT` collision layer will be added                                                   |
| `thickness DEPTH` (optional) | If specified, the quad's thickness will be set to `depth`. Defaults to 0, which results in one sided collision. |
| `CL_X` (optional)            | Space-separated list of colllision layer names. Only those prefixed with `CL_` are used.                        |

## Notes

Game objects can collide if they share a collision layer. Some layers are also
used for certain checks by the game. Static collision is only collidable if it
is in the same [room](./room.md)

If no collision layers are specified, the defaults are `STATIC`, `TANGIBLE`, and
`BLOCK_BALL`. The possible layers and their uses are as follows.

* `STATIC`: Used for portal raycasts, and by:
    * [Doors](./door.md) (when closed)
    * [Dynamic boxes](./dynamic_box.md)
    * [Elevators](./elevator.md)
    * [Fizzler](./fizzler.md) frames
    * Portals
    * [Turret](./turret.md) shots
* `TRANSPARENT`: Unused
* `TANGIBLE`: Used to check grabbing and walking, and by:
    * [Ball catchers](./ball_catcher.md)
    * [Ball launchers](./ball_launcher.md)
    * [Buttons](./button.md)
    * [Decor](./decor.md) (with collision)
    * [Doors](./door.md) (when closed)
    * [Dynamic boxes](./dynamic_box.md)
    * [Elevators](./elevator.md)
    * [Fizzler](./fizzler.md) frames
    * Player
    * Portals (but not portal projectiles)
    * [Security cameras](./security_camera.md)
    * [Switches](./switch.md)
    * [Triggers](./trigger.md)
* `GRABBABLE`: Used to check grabbable and interactable objects, and by:
    * [Decor](./decor.md) (with collision)
    * [Security cameras](./security_camera.md) (when detached)
    * [Switches](./switch.md)
* `FIZZLER`: Used to block portal raycasts, and by:
    * [Decor](./decor.md) (with collision)
    * [Fizzlers](./fizzler.md)
    * Player
    * [Security cameras](./security_camera.md)
* `BLOCK_PORTAL`: Used for portal raycasts, and by:
    * [Buttons](./button.md)
    * [Fizzlers](./fizzler.md)
    * [Switches](./switch.md)
* `BLOCK_BALL`: Used by:
    * [Doors](./door.md) (when closed)
    * Energy balls
    * [Ball launchers](./ball_launcher.md)
    * [Dynamic boxes](./dynamic_box.md)
    * [Elevators](./elevator.md)
    * [Fizzler](./fizzler.md) frames
    * Player
* `BLOCK_TURRET_SHOTS`: Used to block turret shots, and by:
    * [Decor](./decor.md) (with collision)
    * Player
    * [Security cameras](./security_camera.md) (when detached)
    * [Switches](./switch.md)

## TODO

* Page on collision detection
