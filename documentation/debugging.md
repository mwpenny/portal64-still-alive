# Debugging

Portal 64 can be debugged with either an emulator or original hardware.
Emulation is the easiest to set up and works well for high-level development,
but original hardware is still necessary for things like low-level changes,
console-only bugs, performance testing, and sanity checks.

## Debugging Using Emulation

When debugging using emulation, the emulator runs a GDB server for remote
debugging. GDB then connects to the emulator.

1. Download/install the [Ares](https://ares-emu.net/) emulator
2. In Ares, enable the GDB server
    - Settings > Debug > "Enabled" checkbox
    - Optionally configure the port number (default 9123)
3. Build the game and run the resulting ROM in Ares. See
   [Building the Game](./building/building.md).
4. GDB can now be connected. Follow the [GDB instructions](#gdb-setup) below.

## Debugging Using Original Hardware

When debugging using original hardware, the game is compiled with additional
code that implements the GDB protocol and can communicate over a flashcart's USB
serial connection. The connected PC runs a GDB server which acts as a bridge
between GDB and the flashcart.

**You will need either a 64Drive, EverDrive, or SC64.**

1. Download/install [UNFLoader](https://github.com/buu342/N64-UNFLoader).
2. Configure CMake with `DEBUGGER=ON` to include the N64-side debugger code:
    ```sh
    cd portal64

    # Replace <build_directory> with build directory name
    cmake -DDEBUGGER=ON <build_directory>
    ```
3. Build the game. See [Building the Game](./building/building.md).
4. Connect your PC to the flashcart using USB
5. Start the game. UNFLoader can upload it over USB
   (required initial console state depends on flashcart, see
   [UNFLoader documentation](https://github.com/buu342/N64-UNFLoader/blob/master/UNFLoader/README.md#how-to-use-unfloader)):
   ```sh
   UNFLoader -r path/to/portal.z64
   ```
6. When the game boots, it will pause until GDB connects. Start UNFLoader in
   debug mode to start its GDB server:
   ```sh
   # Replace <port> with desired port number, or omit to use default (8080)
   UNFLoader -g <port>
   ```
7. GDB can now be connected. Follow the [GDB instructions](#gdb-setup) below.
8. When finished debugging (GDB disconnected, console turned off, etc.), the
   UNFLoader GDB server will need to be restarted before debugging again.

### Display List Validator

When hardware debugging is enabled, the game can optionally perform sanity
checks on display lists. If a display list is deemed to be invalid, the game
will output an error message to the UNFLoader terminal and intentionally crash.

1. Clone the repository at https://github.com/lambertjamesd/gfxvalidator
2. Copy the `gfxvalidator/gfxvalidator/` directory to `<portal64>/src/`. If done
   correctly, the directory structure should look like:
   ```
   portal64/
   └── src/
       └── gfxvalidator/
           ├── command_printer.c
           ├── command_printer.h
           ├── error_printer.c
           ├── error_printer.h
           ├── gfx_macros.h
           ├── validator.c
           └── validator.h
   ```
3. Configure CMake with `GFX_VALIDATOR=ON` to include the display list validator
   code:
    ```sh
    cd portal64

    # Replace <build_directory> with build directory name
    cmake -DGFX_VALIDATOR=ON <build_directory>
    ```
4. Build the game. See [Building the Game](./building/building.md).
    - There will be compile errors when building for the first time since there
      is no C standard library and the validator includes `string.h`.
    - Remove `#include <string.h>` from all gfxvalidator C files and add the
      following to the top of `error_printer.c`:
      ```c
      #include "util/string.h"

      #define strlen strLength
      ```
5. Debug as described above.

## GDB Setup

Regardless of emulation or original hardware, debugging is done using
[GDB](https://sourceware.org/gdb/). Specifically, a version is required which
supports the N64's `mips:4300` architecture. For example, on Debian/Ubuntu you
can install the `gdb-multiarch` package.

```sh
apt install gdb-multiarch
```

### Command-Line Usage

When launching GDB, provide the path to the built ELF file as an argument.
The file must be the ELF file, not the N64 ROM. **Do not use the `.z64` file.**

```sh
# Replace <build_directory> with build directory name
gdb-multiarch <build_directory>/portal
```

After GDB is launched, ensure the target architecture is properly set:

```sh
set architecture mips:4300
```

Finally, connect to the running GDB server:

```sh
# Replace <port> with port number
target remote localhost:<port>
```

Debugging with GDB once it is connected is beyond the scope of this document.

### Graphical Usage

Optionally, [VSCode](https://code.visualstudio.com/) can be used for a graphical
debugging experience. The
[C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
extension includes support for both the C programming language and debugging
with GDB. Example launch configuration:

```json
// .vscode/launch.json
{
    "name": "Debug with GDB",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/build/portal",
    "cwd": "${workspaceFolder}",
    "MIMode": "gdb",
    "miDebuggerPath": "/path/to/gdb-multiarch",
    "miDebuggerServerAddress": "localhost:<port>",
    "setupCommands": [{
        "text": "set architecture mips:4300"
    }]
}
```

Debugging with VSCode once it is connected is beyond the scope of this document.

It is also possible to define tasks to launch Ares or UNFLoader before starting
GDB. For example:

```json
// .vscode/tasks.json
{
    "label": "Run in Ares",
    "type": "shell",
    "command": "/path/to/ares",
    "args": [
        "--system", "n64",
        "${workspaceFolder}/build/portal.z64"
    ],
    "isBackground": true,

    // Dummy problem matcher so we can debug while running in background
    "problemMatcher": {
        "pattern": [{
            "regexp": ".",
            "file": 1,
            "location": 2,
            "message": 3
        }],
        "background": {
            "activeOnStart": true,
            "beginsPattern": ".",
            "endsPattern": "."
        }
    }
},
{
    "label": "Run on N64",
    "type": "shell",
    "command": "/path/to/UNFLoader",
    "args": [
      "-b",
      "-r", "${workspaceFolder}/build/portal.z64"
    ],
    "problemMatcher": []
},
{
    "label": "Run on N64 for Debugging",
    "type": "shell",
    "command": "/path/to/UNFLoader",
    "args": [
      "-b",
      "-d",
      "-g", "9123"
    ],
    "dependsOn": "Run on N64",
    "isBackground": true,

    // Dummy problem matcher so we can debug while running in background
    "problemMatcher": {
        "pattern": [{
            "regexp": ".",
            "file": 1,
            "location": 2,
            "message": 3
        }],
        "background": {
            "activeOnStart": true,
            "beginsPattern": ".",
            "endsPattern": "."
        }
    }
}
```

Then, in `launch.json` specify the task to run before debugging by adding the
following property to the GDB launch configuration:
```json
"preLaunchTask": "<Name of task>"
```

Tasks can also be run manually by pressing CTRL+SHIFT+P, typing "Run Task", and
selecting a task.
