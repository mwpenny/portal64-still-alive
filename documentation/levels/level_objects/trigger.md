# @trigger

A 3D box that can set [signals](../signals.md) and start
[cutscenes](../cutscenes.md) in response to object collision.

## Name structure

```
@trigger PLAYER_CUTSCENE_NAME PLAYER_SIGNAL_NAME CUBE_CUTSCENE_NAME CUBE_SIGNAL_NAME
```

## Arguments

| Name                              | Description                                                                                                                                                                                                        |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `PLAYER_CUTSCENE_NAME` (optional) | The name of the cutscene to start when the player enters the trigger. Use `-1` to indicate no cutscene.                                                                                                            |
| `PLAYER_SIGNAL_NAME` (optional)   | The name of the signal to emit while the player is in the trigger. Use `-1` to indicate no signal, or omit if not using cube cutscene/signal.                                                                      |
| `CUBE_CUTSCENE_NAME` (optional)   | The name of the cutscene to start when a cube enters the trigger. Prefix with `HOVER_` to require the player to be holding the cube. Use `-1` to indicate no cutscene, or omit if not using cube signal.           |
| `CUBE_SIGNAL_NAME`   (optional)   | The name of the signal to emit while a cube is in the trigger. If the cube cutscene name begins with `HOVER_` the player must also be holding the cube to emit the signal. Use `-1` or omit to indicate no signal. |

## Notes

Triggers are only activated when objects are entirely contained within them.

Signals are set until the relevant object leaves the trigger. Each cutscene
(player or cube) is only activiated at most one time each.