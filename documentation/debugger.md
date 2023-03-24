# Debugger

To get the debugger working with portal64 you should clone these two repos somewhere on your harddrive

[https://github.com/lambertjamesd/libultragdb](https://github.com/lambertjamesd/libultragdb)

[https://github.com/lambertjamesd/gfxvalidator](https://github.com/lambertjamesd/gfxvalidator)

You then need to create the following symbolic links in the root portal64 directory

```bash
~/portal64$ ln -s /path/to/libultragdb/debugger debugger
~/portal64$ ln -s /path/to/gfxvalidator/debugger gfxvalidator
```

If done correctly, the folder structure should look like

```
portal64/
    debugger/
        debugger_stub.c
        debugger.c
        debugger.h
        rsp.c
        rsp.h
        serial.c
        serial.h
    gfxvalidator/
        command_printer.c
        command_printer.h
        error_printer.c
        error_printer.h
        gfx_macros.h
        validator.c
        validator.h
```

Next, you need to configure an environment variable. Put this in your `~/.bashrc` or `~/.profile`

```
export PORTAL64_WITH_DEBUGGER=1
```

Then to build the rom with the debugger in it, use ths make command

```
PORTAL64_WITH_GFX_VALIDATOR=1 make build/portal_debug.z64
```

This will build a version of the game that has a debugger installed. When the game boots, it will pause and wait for something to connect to the debugger before continuing. To connect the debugger with an everdrive, you will need to run the following script.

```
node /path/to/libultragdb/proxy/proxy.js /dev/ttyUSB0 8080
```

This listens on port 8080 for gdb to connect and when it does, it relays connection to the everdrive found at /dev/ttyUSB0. If you get any permssions errors accessing ttyUSB0, change them before trying to debug

```
chmod 666 /dev/ttyUSB0
```

Once the proxy is running you can then connect gdb to port 8080. You will want to use `gdb-multiarch` since this is debugging a MIPS cpu. You can install this version of gdb using apt

```
sudo apt install gdb-multiarch
```

I use vscode to debug with [GDB Debugger - Beyond](https://marketplace.visualstudio.com/items?itemName=coolchyni.beyond-debug) with this launch configuration

`.vscode/launch.json`
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "by-gdb",
            "request": "launch",
            "name": "Debug with EV",
            "program": "${workspaceFolder}/build/portal_debug.elf",
            "cwd": "${workspaceRoot}",
            "debuggerPath": "gdb-multiarch",
            "remote": {
                "enabled": true,
                "address": ":8080",
                "mode": "remote",
                "execfile": "${workspaceFolder}/build/portal_debug.elf"
            },
            "commandsBeforeExec": [
                "set arch mips:4300",
            ],
            "preLaunchTask": "Run EV delay",
        }
    ]
}
```

This expects the task configuration to look like this with the `preLaunchTask` `Run EV Delay`

`.vscode/tasks.json`
```json
{
    "version": "2.0.0",
    "tasks": [
      {
        "label": "Make Debug Rom",
        "type": "shell",
        "command": "PORTAL64_WITH_GFX_VALIDATOR=1 make build/portal_debug.z64",
        "group": "build"
      },
      {
        "label": "Fix Debug Rom",
        "type": "shell",
        "command": "${workspaceFolder}/tools/romfix64.exe ${workspaceFolder}/build/portal_debug.z64",
        "group": "build",
        "dependsOn": "Make Debug Rom"
      },
      {
        "label": "Upload Debug to EV",
        "type": "shell",
        "command": "/home/james/everdrive/usb64/usb64/bin/Debug/usb64.exe -rom=${workspaceFolder}/build/portal_debug.z64 -start",
        "group": "build",
        "dependsOn": "Fix Debug Rom"
      },
      {
        "label": "Run EV delay",
        "type": "shell",
        "command": "sleep 3",
        "group": "build",
        "dependsOn": [
          "Upload Debug to EV"
        ]
      },
      {
        "label": "Start EV Proxy",
        "type": "shell",
        "command": "node /path/to/libultragdb/proxy/proxy.js /dev/ttyUSB0 8080",
        "problemMatcher": [
          "$tsc-watch"
        ],
        "isBackground": true
      }
    ]
}
```

Where the executable usb64.exe comes from here [https://krikzz.com/pub/support/everdrive-64/x-series/dev/](https://krikzz.com/pub/support/everdrive-64/x-series/dev/). It is a windows executable but is compatible with [wine](https://www.winehq.org/). I am running wine-8.0 which you need to install manually instead of using apt.

romfix64.exe is also a windows executable compatible with wine.


I also have a task `Start EV Proxy` that starts the proxy script for connecting gdb to the evedrive. So my processing for debuging on hardware goes as follows.

1. run the `Start EV Proxy` task (ctrl + shift + p, then type 'Run Task', the Select the Start EV Proxy task)
1. turn on the console with the everdrive installed and the usb cable connected
1. select Debug with EV in the run and debug tab in vscode
1. Press F5

If everything is working correctly the rom will automatically be uploaded to the everdrive and the debugger will connect shortly after that. The debugger will always pause after the debugger is connected and you will need to press continue for the game to run. After you turn off the console you will need to stop the proxy and restart it before debugging again. I have tried to make the proxy automatically start and stop without much success so it is still manual.