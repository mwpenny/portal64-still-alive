# @trigger

A 3D box that can set [signals](../signals.md) and start
[cutscenes](../cutscenes/README.md) in response to object collision.

## Name Structure

```
@trigger [touch] [PLAYER_CUTSCENE_NAME] [PLAYER_SIGNAL_NAME] [CUBE_CUTSCENE_NAME] [CUBE_SIGNAL_NAME]
```

## Arguments

| Name                              | Description                                                                                                                                                                                                        |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `touch` (optional)                | If specified, the trigger will activate with any amount of object collision. If omitted, objects must be entirely contained.                                                                                       |
| `PLAYER_CUTSCENE_NAME` (optional) | The name of the cutscene to start when the player enters the trigger. Use `-1` to indicate no cutscene.                                                                                                            |
| `PLAYER_SIGNAL_NAME` (optional)   | The name of the signal to emit while the player is in the trigger. Use `-1` to indicate no signal, or omit if not using cube cutscene/signal.                                                                      |
| `CUBE_CUTSCENE_NAME` (optional)   | The name of the cutscene to start when a cube enters the trigger. Prefix with `HOVER_` to require the player to be holding the cube. Use `-1` to indicate no cutscene, or omit if not using cube signal.           |
| `CUBE_SIGNAL_NAME`   (optional)   | The name of the signal to emit while a cube is in the trigger. If the cube cutscene name begins with `HOVER_` the player must also be holding the cube to emit the signal. Use `-1` or omit to indicate no signal. |

## Notes

Signals are set until the relevant object leaves the trigger.

Each cutscene (player or cube) is only activiated at most one time each.
