# Strings

Most game text is read from the original game's files, but some strings are
unique to Portal 64. Both are combined together for use by the game, with
optional localization.

## String Sources

There are five sources of strings from the original game:

1. `vpk/Portal/hl2/resource/closecaption_<language>.txt`: Captions re-used from Half-Life 2.
2. `vpk/Portal/hl2/resource/gameui_<language>.txt`: Strings related to menus.
3. `vpk/Portal/hl2/resource/valve_<language>.txt`: Miscellaneous non-game-specific strings.
4. `vpk/Portal/portal/resource/closecaption_<language>.txt`: Captions for Portal dialogue and sounds.
5. `vpk/Portal/portal/resource/portal_<language>.txt`: Miscellaneous Portal strings (UI and credits).

Strings unique to Portal 64 are stored in `assets/translations/extra_<language>.txt`.

All Portal "caption" and "extra" strings are included in the game. Other strings
are manually specified in the
[`tools/text/generate_strings.py`](../../tools/text/generate_strings.py) script.

Strings are localized. The build-time text language configuration determines
which are included in the game. See
[documentation/building/building.md](../building/building.md#optional-settings)
for more information.

## String Generation

At build time, the
[`tools/text/generate_strings.py`](../../tools/text/generate_strings.py) script
generates `strings_<language>.c` for each supported language, containing its strings.

It also generates lookup tables (`strings.h`/`strings.c`) to allow the game to
refer to strings at runtime, determine which text languages are supported, and
map localized strings to the player's selected language. The script checks
whether necessary characters are missing from the DejaVu Sans font and will
throw an error if they are (see [Fonts](./fonts.md) for details on updating
fonts).

Generated code is output to `<build directory>/codegen/assets/strings/`.
