# Level signals

Signals are a way to indicate that something has happened in a level. They can be emitted by:
* Level objects
* Triggers
* Cutscenes
* Signal operators

Other parts of the level can listen for signals in order to perform some sort of action. The possible listeners are:
* Level objects
* Cutscenes

Signal information is output into generated level definition C source files at build time by the Lua scripts in `tools/level_scripts/`, initiated by `export_level.lua`.

The code for sending and receiving signals is in `src/scene/signals.c`. The engine uses this to wire up the various emitters and listeners.

## Level objects

Level objects which send or receive signals have the relevant signal name as part of their object name in the level's `.blend` file using the format:
```
@object_type signal_name
```

For example, consider the following object names:
* `@ball_catcher exit_activate`
* `@door exit_activate`

In this example, the ball catcher sets the `exit_activate` signal when activated. The door listens for that signal, and so it will open when a ball reaches the ball catcher.

There are various other level objects that can send and receive signals, including those that the player cannot directly interact with such as the indicator light strips (they are materials). Buttons are special cases and can set two possible signals: one for a regular press and another for a press with a cube. This is useful for only progressing a cutscene once a level is properly solved.

Objects which can be deactivated will clear their associated signal when that occurs.

## Triggers

Triggers are volumes in a level that set signals depending on which object type intersects with them. Like level objects, this information is defined in their object name in the level's `.blend` file. Trigger object names are of the form:
```
@trigger PLAYER_CUTSCENE_NAME player_signal_name CUBE_CUTSCENE_NAME cube_signal_name
```
`player_signal_name` is set/unset when the player enters/leaves the trigger and `cube_signal_name` is set/unset when a cube enters/leaves the trigger.

For more information on triggers, see [Triggers](./level_objects/triggers.md).

## Cutscenes

Cutscenes can set or clear signals using the `set_signal` and `clear_signal` steps, respectively. For example:
```
set_signal door_activate
[...]
clear_signal door_activate
```

Cutscenes can also wait for a particular signal to occur using the `wait_for_signal` step. A number of frames to wait after the signal is set can optionally be specified. For example:
```
[...]
wait_for_signal player_in_trigger
clear_signal door_activate
[...]
wait_for_signal player_in_trigger 42
clear_signal door_activate
[...]
```

For more information on cutscenes, see [Cutscenes](./cutscenes.md).

## Signal operators

Level definition YAML files can optionally contain an array property named `operators`. Each array entry must be an equation matching one of the following forms:
```
signal_name = not other_signal_name
signal_name = signal_a or signal_b
signal_name = signal_c and signal_d
```

Only one `not`, `or`, or `and` operator is allowed per entry. If more complex combinations are required, operators can use signals defined by previous operators. For example:
```
door_not_activated = not door_activated
cube_not_in_room = not cube_in_room
trapped = door_not_activated and cube_not_in_room
```

Operators can use signals from any source.
