# show_prompt

Displays a button prompt informing the player how to perform an action.

## Syntax

```
show_prompt PROMPT_TYPE
```

## Arguments

| Name          | Description                |
| ------------- | -------------------------- |
| `PROMPT_TYPE` | The type of prompt to show |

## Notes

The possible prompt types are:

| Type                        | Description                                                       |
| --------------------------- | ----------------------------------------------------------------- |
| `CutscenePromptTypeNone`    | Remove the current prompt                                         |
| `CutscenePromptTypePortal0` | Prompt to fire the orange portal                                  |
| `CutscenePromptTypePortal1` | Prompt to fire the blue portal                                    |
| `CutscenePromptTypePickup`  | Prompt to pick up item                                            |
| `CutscenePromptTypeDrop`    | Prompt to drop item                                               |
| `CutscenePromptTypeUse`     | Prompt to use object (e.g., [switch](../level_objects/switch.md)) |
| `CutscenePromptTypeCrouch`  | Prompt to crouch                                                  |
| `CutscenePromptTypeMove`    | Prompt to move                                                    |
| `CutscenePromptTypeJump`    | Prompt to jump                                                    |

When a prompt is shown, the currently mapped button(s) for the action are used.

Performing a prompt's action will remove it if on screen and prevent it from
being shown until either a save file or new level are loaded. This is true even
if the action takes place before the prompt is first displayed.

Only one prompt can be shown at a time. Showing a new one will remove any
currently displayed prompt.
