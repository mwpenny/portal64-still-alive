# Cutscenes

Cutscenes are scripted sequences of steps which can be executed by the game
in response to [triggers](../level_objects/trigger.md) or other cutscenes. Unlike
typical usage of the term "cutscene", most do not take control away from the
player and are instead more similar to background threads.

## Definition

At level export time, level definition C source files are generated using the
Lua scripts in `tools/level_scripts/`, initiated by `export_level.lua`. Cutscene
information is written using information from each level's associated
[YAML file](../file_formats.md#levels).

In the YAML file, cutscenes are defined in an object property called `cutscenes`.
Property names are used to reference cutscenes from triggers or other cutscenes.
Property values are lists of steps to perform.

Steps use the syntax:
```
STEP_NAME [ARG]...
```

That is, the step name followed by 0 or more space-separated arguments. The
number and type of the arguments is specific to each step type.

For example,
```yaml
cutscenes:
  OPEN_PORTAL:
    - delay 5
    - rumble 0
    - open_portal back_wall 0

  ENTERED_TRIGGER:
    - set_signal open_door
```

Multiple cutscenes can be running at once, and triggered cutscenes are stopped
after their final step is executed.

## Step types

See the pages below for details on specific cutscene steps.

| Type                                          | Description                                 |
| --------------------------------------------- | ------------------------------------------- |
| [play_sound](./play_sound.md)                 | Plays a sound and waits for completion      |
| [start_sound](./start_sound.md)               | Plays a sound without waiting               |
| [q_sound](./q_sound.md)                       | Queues a sound to be played on a channel    |
| [wait_for_channel](./wait_for_channel.md)     | Waits for a sound channel to be available   |
| [delay](./delay.md)                           | Pauses for a specified duration             |
| [open_portal](./open_portal.md)               | Opens a portal at a location                |
| [close_portal](./close_portal.md)             | Closes a portal at a location               |
| [set_signal](./set_signal.md)                 | Changes a signal's default state to set     |
| [clear_signal](./clear_signal.md)             | Changes a signal's default state to unset   |
| [wait_for_signal](./wait_for_signal.md)       | Waits for a signal to become set            |
| [teleport_player](./teleport_player.md)       | Moves the player to a location              |
| [load_level](./load_level.md)                 | Loads the next level                        |
| [label](./label.md)                           | Marker for goto                             |
| [goto](./goto.md)                             | Unconditional jump to a label               |
| [start_cutscene](./start_cutscene.md)         | Starts a cutscene                           |
| [stop_cutscene](./stop_cutscene.md)           | Stops a cutscene                            |
| [wait_for_cutscene](./wait_for_cutscene.md)   | Waits for a cutscene to end                 |
| [hide_pedestal](./hide_pedestal.md)           | Hides pedestals and grants the portal gun   |
| [point_pedestal](./point_pedestal.md)         | Points pedestals at a location              |
| [play_animation](./play_animation.md)         | Plays or updates an armature's animation    |
| [pause_animation](./pause_animation.md)       | Pauses playback of an armature's animation  |
| [resume_animation](./resume_animation.md)     | Resumes playback of an armature's animation |
| [wait_for_animation](./wait_for_animation.md) | Waits until an armature is not animated     |
| [save_checkpoint](./save_checkpoint.md)       | Creates an autosave                         |
| [damage_player](./damage_player.md)           | Damages the player                          |
| [show_prompt](./show_prompt.md)               | Displays an action prompt                   |
| [rumble](./rumble.md)                         | Activates the rumble pak                    |
| [activate_signage](./activate_signage.md)     | Turns on test chamber signs                 |
