# Level Signals

Signals are a way to indicate that something is happening in a level. They can
be controlled by:
* Level objects
* Triggers
* Cutscenes
* Signal operators

Emitting a signal sets its state to the inverse of its default state.

Other parts of the level can listen for signals in order to perform some sort of
action. The possible listeners are:
* Level objects
* Cutscenes

Signal information is output into generated level definition C source files at
level export time by the Lua scripts in `tools/level_scripts/`, initiated by
`export_level.lua`.

The code for sending and receiving signals is in `src/scene/signals.c`. The
engine uses this to wire up the various emitters and listeners.

## Level objects

Level objects which send or receive signals have the relevant signal name as
part of their object name in the level's `.blend` file.

For example, consider the following object names:
* `@ball_catcher exit_activate`
* `@door exit_activate`

In this example, the [ball catcher](./level_objects/ball_catcher.md) sets the
`exit_activate` signal when activated. The [door](./level_objects/door.md)
listens for that signal, and so it will open when a ball reaches the ball catcher.

There are various other level objects that can send and receive signals,
including those that the player cannot directly interact with such as the
indicator light strips. Some other special cases, like
[buttons](./level_objects/button.md), can set multiple signals. The exact name
structure differs per object. See [Level Objects](./level_objects/README.md) for
more details.

Objects which can be deactivated will stop emitting their associated signal when
that occurs.

## Triggers

[Triggers](./level_objects/trigger.md) are 3D boxes in a level that set signals
depending on which object type enters them. Like level objects, this information
is defined in their object name in the level's `.blend` file. Trigger object
names are of the form:
```
@trigger PLAYER_CUTSCENE_NAME PLAYER_SIGNAL_NAME CUBE_CUTSCENE_NAME CUBE_SIGNAL_NAME
```
`PLAYER_SIGNAL_NAME` is emitted while the player is inside the trigger and
`CUBE_SIGNAL_NAME` is emitted while a cube is inside the trigger.

## Cutscenes

[Cutscenes](./cutscenes/README.md) can change the default state of signals using
the [set_signal](./cutscenes/set_signal.md) and
[clear_signal](./cutscenes/clear_signal.md) steps, respectively. For example:
```
set_signal door_activate
[...]
clear_signal door_activate
```

In between `set_signal` and `clear_signal`, the `door_activate` signal will be
read as set. Since emitting a signal inverts the default state, signal emitters
such as buttons will change the state to unset when activated.

Cutscenes can also wait for a particular signal to become set using the
[wait_for_signal](./cutscenes/wait_for_signal.md) step. A number of frames the
signal must be set for can optionally be specified. For example:
```
[...]
wait_for_signal player_in_trigger
clear_signal door_activate
[...]
wait_for_signal player_in_trigger 42
clear_signal door_activate
[...]
```

## Signal operators

Level definition YAML files can optionally contain an array property named
`operators`. Each array entry must be an equation matching one of the following
forms:
```
signal_name = not other_signal_name
signal_name = signal_a or signal_b
signal_name = signal_c and signal_d
```

Only one `not`, `or`, or `and` operator is allowed per entry. If more complex
combinations are required, operators can use signals defined by previous
operators. For example:
```yaml
operators:
  - door_not_activated = not door_activated
  - cube_not_in_room = not cube_in_room
  - trapped = door_not_activated and cube_not_in_room
```

Operators are evaluated from top to bottom to update their corresponding
signals. Signals from any source can be used.
