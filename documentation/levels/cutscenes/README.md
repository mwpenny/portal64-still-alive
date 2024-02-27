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

* [play_sound](./play_sound.md)
* [start_sound](./start_sound.md)
* [q_sound](./q_sound.md)
* [wait_for_channel](./wait_for_channel.md)
* delay (TODO)
* open_portal (TODO)
* close_portal (TODO)
* set_signal (TODO)
* clear_signal (TODO)
* wait_for_signal (TODO)
* teleport_player (TODO)
* load_level (TODO)
* label (TODO)
* goto (TODO)
* start_cutscene (TODO)
* stop_cutscene (TODO)
* wait_for_cutscene (TODO)
* hide_pedestal (TODO)
* point_pedestal (TODO)
* play_animation (TODO)
* pause_animation (TODO)
* resume_animation (TODO)
* wait_for_animation (TODO)
* save_checkpoint (TODO)
* kill_player (TODO)
* show_prompt (TODO)
* rumble (TODO)
* activate_signage (TODO)
