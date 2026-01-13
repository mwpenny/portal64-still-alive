#!/usr/bin/env python3

"""
This script converts controller inputs captured while debugging into a header
file that can be included in the game to play them back.

Usage:
1. Build the game with hardware debugging support and CONTROLLER_LOGGING set to
   CONTROLLER_LOGGING_RECORD in controller_<library>.c.

2. Debug the game to generate *.bin files containing controller data.

3. When finished playing, run this script on the directory containing the *.bin
   files saved during the debugging session. Save the resulting header file
   alongside controller_<library>.c as controller_recording.h.

4. Build the game with CONTROLLER_LOGGING set to CONTROLLER_LOGGING_PLAYBACK.
   The previously-recorded actions will be played back in-game. Manual
   controller inputs will be ignored until playback finishes.

Caveat:

While the recorded input structure is platform-agnostic, the data saved in the
structure differs (e.g., libultra/libdragon button bits are reversed). In
other words, this script is generic but recording and playback must be done
on the same platform.
"""

import argparse
import os
import pathlib
import struct

CONTROLLER_DATA_FILE_PATTERN = "*.bin"

# Code generation

def parse_controller_data_file(file_path):
    with open(file_path, "rb") as f:
        data = f.read()
        if len(data) < 8:
            return None

        # Data files contain data for 2 controllers (4 bytes each)
        parsed = []
        for i in range(0, 8, 4):
            buttons, stick_x, stick_y = struct.unpack(">H2b", data[i:i+4])
            parsed.append(f"{{ {buttons}, {{ {stick_x}, {stick_y} }} }}")
        return parsed

def parse_controller_data(file_dir):
    all_parsed_data = []

    for file in pathlib.Path(file_dir).glob(CONTROLLER_DATA_FILE_PATTERN):
        parsed = parse_controller_data_file(file)
        if parsed:
            all_parsed_data += parsed

    return all_parsed_data

def generate_header(controller_data):
    controller_data_entries = "\n".join(
        f"    {data}," for data in controller_data
    )

    return (
        f"struct RecordedControllerData {{\n"
        f"    enum ControllerButtons buttons;\n"
        f"    struct ControllerStick stick;\n"
        f"}};\n"
        f"\n"
        f"struct RecordedControllerData gRecordedControllerData[] = {{\n"
        f"{controller_data_entries}\n"
        f"}};\n"
    )

# Main

def write_string(output_file, s):
    output_file = os.path.abspath(output_file)
    output_dir = os.path.dirname(output_file)

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(s)

def get_args():
    parser = argparse.ArgumentParser(
        prog="parse_controller_data",
        description="Generates C code containing recorded controller data"
    )
    parser.add_argument(
        "input_dir",
        metavar="INPUT_DIR",
        help="Directory containing controller data .bin files"
    )
    parser.add_argument(
        "output_file",
        metavar="OUTPUT_FILE",
        help="Output header ile name"
    )

    return parser.parse_args()

args = get_args()

controller_data = parse_controller_data(args.input_dir)
write_string(
    args.output_file,
    generate_header(controller_data)
)
