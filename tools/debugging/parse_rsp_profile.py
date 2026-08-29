#!/usr/bin/env python3

"""
This script parses RSP profiler output to generate a summary of frame
performance hot spots.

Usage:

1. Build the game with hardware debugging support and the RSP profiler enabled.

2. While debugging the game, press d-pad down on controller 3 to output display
   list timing information to the terminal for the last frame rendered.

3. Run this script with the debug log file as an argument. It will output the
   captured display list commands in order from most to least expensive. The log
   file is also used to label dynamic model display lists. Optionally provide
   portal.map from the build directory as a second argument to label other
   display lists as well.

Caveats:

* Only commands from the top-level display list are present in the final output.
  The profiler does not recursively measure child performance.

* Dynamically loaded elements such as level geometry can share addresses with
  each other. Such display lists are labelled using a comma-separated list of
  all matching symbol names.

* The game builds some display lists at runtime, meaning they don't have symbol
  names. They are labelled using a +-separated list of all named child display
  lists (recursively). The top-level address is used when no names are found.
"""

import argparse
import re

BEGIN_PROFILE_REGEX = re.compile(r"^Begin RSP profile$")

class SymbolMap:
    STATIC_SYMBOL_REGEX = re.compile(r"^\s+0x([a-f0-9]+)\s+(\w+)$")
    DYNAMIC_ASSET_RESET_REGEX = re.compile(r"^Reset dynamic assets$")
    DYNAMIC_ASSET_LOAD_REGEX = re.compile(r"^Loaded dynamic asset at 0x([a-f0-9]{8}): (\w+)$")

    def __init__(self, file_path):
        self._static_symbols = file_path and self._parse_symbol_file(file_path) or dict()
        self._dynamic_symbols = dict()

    @staticmethod
    def _parse_static_symbol(line):
        match = SymbolMap.STATIC_SYMBOL_REGEX.match(line)
        if not match:
            return None, None

        address, name = match.groups()
        return int(address, 16), name

    @staticmethod
    def _parse_symbol_file(file_path):
        address_to_symbol = dict()

        with open(file_path, "r") as f:
            for line in f:
                address, name = SymbolMap._parse_static_symbol(line)
                if address:
                    # Dynamic elements can share the same address
                    names = filter(None, (address_to_symbol.get(address), name))
                    address_to_symbol[address] = ",".join(names)

        return address_to_symbol

    @staticmethod
    def _is_dynamic_asset_reset_line(line):
        return SymbolMap.DYNAMIC_ASSET_RESET_REGEX.match(line)

    @staticmethod
    def _parse_dynamic_asset_load(line):
        match = SymbolMap.DYNAMIC_ASSET_LOAD_REGEX.match(line)
        if not match:
            return None, None

        address, name = match.groups()
        return int(address, 16), name

    def try_update(self, line):
        if (self._is_dynamic_asset_reset_line(line)):
            self._dynamic_symbols.clear()
            return True

        address, name = self._parse_dynamic_asset_load(line)
        if address:
            self._dynamic_symbols[address] = name
            return True

        return False

    def get_symbol_name(self, address):
        return self._static_symbols.get(address) or \
            self._dynamic_symbols.get(address)


class DisplayList:
    COMMAND_REGEX = re.compile(r"^dl 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8})$")
    G_FILLRECT    = 0xf6
    G_POPMTX      = 0xd8
    G_MTX         = 0xda
    G_MOVEWORD    = 0xdb
    G_DL          = 0xde
    G_ENDDL       = 0xdf

    class Command:
        def __init__(self, command, w0, w1):
            self.command = command
            self.w0 = w0
            self.w1 = w1
            self.children = []
            self.name = None

    def __init__(self, symbol_map):
        self._symbol_map = symbol_map
        self._stack = [None]  # Dummy element for root display list
        self._visited = set()
        self._child_names = dict()

    @staticmethod
    def _parse_command(line):
        match = DisplayList.COMMAND_REGEX.match(line)
        if not match:
            return None

        command, w0, w1 = match.groups()
        return DisplayList.Command(
            int(command, 16),
            int(w0, 16),
            int(w1, 16)
        )

    def _update(self, command):
        if not self._stack:
            raise ValueError("Malformed display list")

        match command.command:
            case DisplayList.G_DL:
                current = self._stack[-1]
                if current:
                    current.children.append(command)
                self._stack.append(command)

            case DisplayList.G_ENDDL:
                dl = self._stack.pop()
                if dl and not (dl in self._visited):
                    name = self._symbol_map.get_symbol_name(dl.w1) or \
                        "+".join(filter(None, (c.name for c in dl.children)))

                    if name:
                        dl.name = name
                        self._child_names[dl.w1] = dl.name

                    self._visited.add(dl.w1)

    def try_update(self, line):
        command = self._parse_command(line)
        if command:
            self._update(command)
            return True

        return False

    def get_command_name(self, command):
        match command.command:
            case DisplayList.G_FILLRECT:
                return "gsDPFillRectangle"
            case DisplayList.G_POPMTX:
                return "gsSPPopMatrix"
            case DisplayList.G_MTX:
                return "gsSPMatrix"
            case DisplayList.G_MOVEWORD:
                segment_num = (command.w0 // 4) & 0xf
                return f"gsSPSegment(0x{segment_num:x}, 0x{command.w1:08x})"
            case DisplayList.G_DL:
                address = self._child_names.get(command.w1) or \
                    f"0x{command.w1:08x}"
                return f"gsSPDisplayList({address})"
            case _:
                return f"unknown 0x{command.command:x} 0x{command.w0:08x}{command.w1:08x}"


class Profile:
    SAMPLE_REGEX = re.compile(r"^(\d+)\/\d+ 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8}) (\d+\.?\d+) ms$")

    class CommandInfo:
        def __init__(self, command_index, command, w0, w1, start_time_ms):
            self.command_index = command_index
            self.command = command
            self.w0 = w0
            self.w1 = w1
            self.start_time_ms = start_time_ms
            self.run_time_ms = 0
            self.sample_count = 1

    def __init__(self, symbol_map):
        self._samples = []
        self._display_list = DisplayList(symbol_map)

    @staticmethod
    def _parse_sample(line):
        match = Profile.SAMPLE_REGEX.match(line)
        if not match:
            return None

        command_index, command, w0, w1, start_time_ms = match.groups()
        return Profile.CommandInfo(
            int(command_index),
            int(command, 16),
            int(w0, 16),
            int(w1, 16),
            float(start_time_ms)
        )

    def try_update(self, line):
        sample = self._parse_sample(line)
        if sample:
            self._samples.append(sample)
            return True

        return self._display_list.try_update(line)

    def get_display_list(self):
        return self._display_list

    def aggregate(self):
        command_stats = dict()

        # Bucket and sum
        for sample in self._samples:
            command = command_stats.get(sample.command_index)
            if command:
                command.start_time_ms += sample.start_time_ms
                command.sample_count += 1
            else:
                command_stats[sample.command_index] = sample

        # Average
        for command in command_stats.values():
            command.start_time_ms /= command.sample_count

        # Compute run time
        max_index = self._samples[-1].command_index
        for i in range(max_index):
            current = command_stats.get(i)
            next = command_stats.get(i + 1)

            if (not current) or (not next):
                raise IndexError(f"Gap in samples at command index {i}")

            current.run_time_ms = next.start_time_ms - current.start_time_ms

        # The last command is always a pipe sync we don't care about
        del command_stats[max_index]
        return sorted(
            command_stats.values(),
            key=lambda s: s.run_time_ms,
            reverse=True
        )

# Main

def format_profile_stats(profile):
    column_padding = [20, 6, 0]
    pad = lambda e: f"{e[1]}".ljust(column_padding[e[0]])

    lines = [
        ["Run time (ms)", "Idx", "Command"],
        ["-------------", "---", "-------"],
        *[[
            f"{c.run_time_ms:.16f}",
            c.command_index,
            profile.get_display_list().get_command_name(c)
        ] for c in profile.aggregate()]
    ]

    return "\n".join(
        " ".join(map(pad, (c for c in enumerate(line))))
        for line in lines
    )

def get_args():
    parser = argparse.ArgumentParser(
        prog="parse_rsp_profile",
        description="Generates frame performance data from RSP profiler output"
    )
    parser.add_argument(
        "debug_log_file",
        metavar="DEBUG_LOG_FILE",
        help="File containing debug console output"
    )
    parser.add_argument(
        "symbol_map_file",
        metavar="SYMBOL_MAP_FILE",
        nargs="?",
        help="Linker-generated file containing symbol names and addresses"
    )

    return parser.parse_args()

args = get_args()

symbol_map = SymbolMap(args.symbol_map_file)
profiles = []

with open(args.debug_log_file, "r") as f:
    for line in f:
        if BEGIN_PROFILE_REGEX.match(line):
            profiles.append(Profile(symbol_map))

        (profiles and profiles[-1].try_update(line)) or symbol_map.try_update(line)

for i, profile in enumerate(profiles):
    print(f"=== Begin profile {i + 1} ===")
    print(format_profile_stats(profile))
    print(f"=== End profile {i + 1} ===")
