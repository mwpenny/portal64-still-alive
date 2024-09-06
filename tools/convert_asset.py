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
    if len(sys.argv) != 5:
        print("Runs a specified command with the given input and output file paths")
        print("The command's arguments are read from a file")
        print()
        print(f"Usage: {sys.argv[0]} COMMAND INPUT_FILE ARGS_FILE OUTPUT_FILE")
        sys.exit(1)

    command, input_file, args_file, output_file = sys.argv[1:]

    output_parent_dir = os.path.dirname(output_file)
    if output_parent_dir:
        os.makedirs(output_parent_dir, exist_ok=True)

    with open(args_file) as f:
        args = shlex.split(f.read().strip())

    rc = subprocess.run([
        command,
        input_file,
        *args,
        output_file
    ])
    sys.exit(rc.returncode)
