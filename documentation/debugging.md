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

### RSP Profiler

When hardware debugging is enabled, the game can optionally output display list
performance information and screenshots. This data can then be used to generate
command cost reports and heatmaps.

1. Configure CMake with `RSP_PROFILER=ON` to include the RSP profiler code:
    ```sh
    cd portal64

    # Replace <build_directory> with build directory name
    cmake -DRSP_PROFILER=ON <build_directory>
    ```
2. Build the game. See [Building the Game](./building/building.md).
3. Debug as described above, but when starting UNFLoader's GDB server, also pass
   the `-d` argument to save console logs to a file. Screenshots will be saved
   to the current working directory.
    ```sh
    # Replace <screenshot_directory> with desired screenshot directory name
    cd <screenshot_directory>

    # Replace <log_file> with desired log file path
    UNFLoader -g -d <log_file>
    ```
4. While debugging, press d-pad down on controller 3 to profile the last
   rendered frame.
5. When finished debugging, pass the log file and screenshot directory paths to the
   [tools/debugging/parse_rsp_profile.js](../tools/debugging/parse_rsp_profile.js)
   script to generate reports and heatmaps for each profiled frame. See the
   script for more information.

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

```jsonc
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

```jsonc
// .vscode/tasks.json
{
    "label": "Run in Ares",
    "type": "shell",
    "command": "/path/to/ares",
    "args": [
        "--system", "Nintendo 64",
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
