#!/usr/bin/env python3

import subprocess
import os
import shlex
import sys

# Arguments to commands for converting assets are stored in text files
#
# This helper script allows calling the commands with the arguments from
# such files in a cross-platform way.

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Runs a specified command with the given input and output file paths")
        print("The command's arguments are read from a file")
        print()
        print(f"Usage: {sys.argv[0]} COMMAND INPUT_FILE ARGS_FILE [ARG]...")
        sys.exit(1)

    command, input_file, args_file, *additional_args = sys.argv[1:]

    with open(args_file) as f:
        args = shlex.split(f.read().strip())

    rc = subprocess.run([
        command,
        input_file,
        *args,
        *additional_args
    ])
    sys.exit(rc.returncode)
