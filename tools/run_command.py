#!/usr/bin/env python3

import subprocess
import shlex
import sys

# Arguments to commands for converting assets are stored in text files
#
# This helper script allows calling the commands with the arguments from
# such files in a cross-platform way.

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Runs a specified command with the given arguments")
        print("Arguments prefixed with '@' are read as files containing arguments")
        print()
        print(f"Usage: {sys.argv[0]} COMMAND [ARG]...")
        sys.exit(1)

    command, *args = sys.argv[1:]
    parsed_args = []

    for arg in args:
        if arg.startswith('@'):
            with open(arg[1:]) as f:
                parsed_args += shlex.split(f.read().strip())
        else:
            parsed_args.append(arg)

    rc = subprocess.run([
        command,
        *parsed_args
    ])
    sys.exit(rc.returncode)
