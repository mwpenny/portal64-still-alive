#!/usr/bin/env python3
import argparse
import json
import os
import re
import sys

sys.stdout.reconfigure(encoding="utf8")

HL2_RESOURCE_DIR = os.path.join("Portal", "hl2", "resource")
PORTAL_RESOURCE_DIR = os.path.join("Portal", "portal", "resource")

# Include only languages the font supports
# In-game lists display in this defined order
SUPPORTED_LANGUAGES = {
    "english":    "English",
    "brazilian":  "Brasileiro",
    "bulgarian":  "Български език",
    "czech":      "Čeština",
    "danish":     "Dansk",
    "german":     "Deutsch",
    "spanish":    "Español",
    "latam":      "Español americana",
    "greek":      "Ελληνικά",
    "french":     "Français",
    "italian":    "Italiano",
    "polish":     "Język polski",
    "hungarian":  "Magyar nyelv",
    "dutch":      "Nederlands",
    "norwegian":  "Norsk",
    "portuguese": "Português",
    "russian":    "Русский язык",
    "romanian":   "Românește",
    "finnish":    "Suomi",
    "swedish":    "Svenska",
    "turkish":    "Türkçe",
    "ukrainian":  "Українська мова",
}

STRINGS_GAMEUI = {
    "GAMEUI_ASPECTWIDE",
    "GAMEUI_AUDIO",
    "GAMEUI_AUTOSAVE",
    "GAMEUI_CANCEL",
    "GAMEUI_CAPTIONING",
    "GAMEUI_CHAPTER",
    "GAMEUI_CONFIRMDELETESAVEGAME_INFO",
    "GAMEUI_CONFIRMDELETESAVEGAME_OK",
    "GAMEUI_CONFIRMDELETESAVEGAME_TITLE",
    "GAMEUI_CONFIRMLOADGAME_TITLE",
    "GAMEUI_CONFIRMNEWGAME_TITLE",
    "GAMEUI_CONFIRMOVERWRITESAVEGAME_INFO",
    "GAMEUI_CONFIRMOVERWRITESAVEGAME_OK",
    "GAMEUI_CONFIRMOVERWRITESAVEGAME_TITLE",
    "GAMEUI_CONSOLE_QUITWARNING",
    "GAMEUI_DELETE",
    "GAMEUI_GAMEMENU_QUIT",
    "GAMEUI_GAMEMENU_RESUMEGAME",
    "GAMEUI_JOYSTICKINVERTED",
    "GAMEUI_LOAD",
    "GAMEUI_LOADGAME",
    "GAMEUI_LOADWARNING",
    "GAMEUI_MUSICVOLUME",
    "GAMEUI_NEWGAME",
    "GAMEUI_NEWGAMEWARNING",
    "GAMEUI_NEWSAVEGAME",
    "GAMEUI_NO",
    "GAMEUI_OPTIONS",
    "GAMEUI_PLAYERNAME",
    "GAMEUI_PORTAL",
    "GAMEUI_PORTALDEPTHLABEL",
    "GAMEUI_PORTALFUNNEL",
    "GAMEUI_QUITCONFIRMATIONTITLE",
    "GAMEUI_SAVE",
    "GAMEUI_SAVEGAME",
    "GAMEUI_SOUNDEFFECTVOLUME",
    "GAMEUI_SUBTITLES",
    "GAMEUI_SUBTITLESANDSOUNDEFFECTS",
    "GAMEUI_USEDEFAULTS",
    "GAMEUI_VIDEO",
    "GAMEUI_YES",
}

STRINGS_VALVE = {
    "VALVE_COMBAT_TITLE",
    "VALVE_DUCK",
    "VALVE_JUMP",
    "VALVE_LOOK_STRAIGHT_AHEAD",
    "VALVE_MISCELLANEOUS_KEYBOARD_KEYS_TITLE",
    "VALVE_MISCELLANEOUS_TITLE",
    "VALVE_MOVEMENT_TITLE",
    "VALVE_PAUSE_GAME",
    "VALVE_USE_ITEMS",
}

STRINGS_PORTAL = {
    "HINT_DROP_ITEMS",
    "HINT_DUCK",
    "HINT_GET_PORTAL_1",
    "HINT_GET_PORTAL_2",
    "HINT_JUMP",
    "HINT_USE_ITEMS",
    "HINT_USE_SWITCHES",
    "PORTAL_CHAPTER1_TITLE",
    "VALVE_PRIMARY_ATTACK",
    "VALVE_SECONDARY_ATTACK",
}

# TODO: whitelist captions

# Validation

def check_font(language_strings, font_data):
    with open(font_data, "r") as f:
        content = json.load(f)
        supported_chars = set(chr(symbol["id"]) for symbol in content["symbols"])

    used_chars = set(
        c for strings in language_strings.values()
        for s in strings.values()
        for c in s
    )
    unused_chars = supported_chars - used_chars

    # TODO: add missing characters to font and make this an error
    invalid_chars = used_chars - supported_chars
    if len(invalid_chars) > 0:
        print(f"Invalid characters ({len(invalid_chars)})\n{''.join(sorted(invalid_chars))}\n")

    print(f"Used characters ({len(used_chars)})\n{''.join(sorted(used_chars))}\n")
    print(f"Unused characters ({len(unused_chars)})\n{''.join(sorted(unused_chars))}\n")

# Code generation

def capitalize(s):
    return s[:1].upper() + s[1:]

def generate_header(language_strings):
    first_language = list(language_strings)[0]

    max_message_length = max(
        len(s) for strings in language_strings.values()
        for s in strings.values()
    )

    language_defines = "\n".join(
        f"#define LANGUAGE_{entry.upper()} {i}"
        for i, entry in enumerate(language_strings)
    )
    string_enum_entries = "\n".join(
        f"    {k.upper()}," for k in language_strings[first_language]
    )

    return (
        f"#ifndef __STRINGS_H__\n"
        f"#define __STRINGS_H__\n"
        f"\n"
        f"#define NUM_STRING_LANGUAGES   {len(language_strings)}\n"
        f"#define NUM_TRANSLATED_STRINGS {len(language_strings[first_language].values()) + 1}\n"
        f"#define MAX_STRING_LENGTH      {max_message_length}\n"
        f"\n"
        f"struct StringBlock {{\n"
        f"    char* romStart;\n"
        f"    char* romEnd;\n"
        f"    char** values;\n"
        f"}};\n"
        f"\n"
        f"extern char* StringLanguages[];\n"
        f"extern struct StringBlock StringLanguageBlocks[];\n"
        f"\n"
        f"{language_defines}\n"
        f"\n"
        f"enum StringId\n"
        f"{{\n"
        f"    StringIdNone,\n"
        f"{string_enum_entries}\n"
        f"}};\n"
        f"\n"
        f"#endif"
    )

def generate_main_source_file(language_strings):
    language_name_entries = "\n".join(
        f'    "{SUPPORTED_LANGUAGES[lang]}",'
        for lang in language_strings
    )

    output = (
        f'#include "strings.h"\n'
        f"\n"
        f"char* StringLanguages[] =\n"
        f"{{\n"
        f"{language_name_entries}\n"
        f"}};\n"
        f"\n"
    )

    for language in language_strings:
        output += (
            f"extern char _strings_{language}SegmentRomStart[];\n"
            f"extern char _strings_{language}SegmentRomEnd[];\n"
            f"extern char* gStrings{capitalize(language)}[NUM_TRANSLATED_STRINGS];\n"
            f"\n"
        )

    output += f"struct StringBlock StringLanguageBlocks[] = {{\n"

    for language in language_strings:
        output += (
            f"    {{\n"
            f"        _strings_{language}SegmentRomStart,\n"
            f"        _strings_{language}SegmentRomEnd,\n"
            f"        gStrings{capitalize(language)},\n"
            f"    }},\n"
        )

    output += f"}};"
    return output

def generate_language_source_file(language, strings, default_strings):
    string_declarations = "\n".join(
        f'char __translation_{language}_{k}[] = "{strings.get(k, v)}";'
        for (k, v) in default_strings.items()
    )
    string_table_entries = "\n".join(
        f'    __translation_{language}_{k},'
        for k in default_strings
    )

    return (
        f'#include "strings.h"\n'
        f"\n"
        f"{string_declarations}\n"
        f"\n"
        f"char* gStrings{capitalize(language)}[NUM_TRANSLATED_STRINGS] = {{\n"
        f'    "",\n'  # StringIdNone
        f"{string_table_entries}\n"
        f"}};"
    )

# Some lines do not end with a quote, and it is placed on the following line
KEY_VALUE_REGEX = re.compile(r'^"(.+)"\s+"(.+)')

def parse_strings_file(filepath, whitelist={}, encoding="utf-16-le"):
    strings = dict()
    found_tokens = False

    with open(filepath, "r", encoding=encoding) as f:
        for line in f:
            line = line.strip()

            if not found_tokens:
                found_tokens = '"tokens"' in line.lower()
                continue
            if ("{" in line) or ("}" in line):
                continue
            if "[english]" in line or "#commentary" in line or "[$X360]" in line:
                continue

            match = KEY_VALUE_REGEX.match(line)
            if not match:
                continue

            key, value = match.groups()

            if key == "gameui_subtitles":
                print(whitelist)
                print(key, value)

            key = key.replace('"', "") \
                .replace(".", "_") \
                .replace("-", "_") \
                .replace("\\", "_") \
                .replace("#", "") \
                .strip() \
                .upper()

            value = value.replace("[$WIN32]", "") \
                .replace("<len>", "") \
                .replace("<sfx>", "") \
                .replace('"', "") \
                .replace("\n", "") \
                .replace("\\n", " ") \
                .replace("\\", "") \
                .strip()
            value = re.sub(r"\<clr.+\>", "", value)
            value = re.sub(r"\<norepeat.+\>", "", value)
            value = re.sub(r"\<len:.+\>", "", value)
            value = re.sub(r"%[^%]*%,?\s", "", value)

            if (not whitelist) or (key in whitelist):
                strings[key] = value

    return strings

def parse_all_languages(game_root_dir, extra_translations_dir, languages):
    language_strings = dict()

    for language in languages:
        strings = {
            # Game UI
            **parse_strings_file(
                os.path.join(game_root_dir, HL2_RESOURCE_DIR, f"gameui_{language}.txt"),
                whitelist=STRINGS_GAMEUI
            ),

            # Valve
            **parse_strings_file(
                os.path.join(game_root_dir, HL2_RESOURCE_DIR, f"valve_{language}.txt"),
                whitelist=STRINGS_VALVE
            ),

            # Captions
            **parse_strings_file(
                os.path.join(game_root_dir, PORTAL_RESOURCE_DIR, f"closecaption_{language}.txt")
            ),

            # Portal
            **parse_strings_file(
                os.path.join(game_root_dir, PORTAL_RESOURCE_DIR, f"portal_{language}.txt"),
                whitelist=STRINGS_PORTAL
            ),

            # Extra
            **parse_strings_file(
                os.path.join(extra_translations_dir, f"extra_{language}.txt"),
                encoding="utf-8"
            )
        }

        language_strings[language] = strings

    return language_strings

# Main

def write_string(output_file, s):
    output_file = os.path.abspath(output_file)
    output_dir = os.path.dirname(output_file)

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(s)

def language_list(s):
    provided_languages = [lang.strip().lower() for lang in s.split(",")]

    if "all" in provided_languages and len(provided_languages) > 1:
        raise ValueError("Cannot specify 'all' with other languages")

    # Sort in consistent order
    return [
        lang for lang in SUPPORTED_LANGUAGES
        if lang in provided_languages or "all" in provided_languages
    ]

def get_args():
    parser = argparse.ArgumentParser(
        prog="generate_strings",
        description="Generates C code containing game string data"
    )
    parser.add_argument(
        "--game-root-dir", required=True,
        help="Root directory of Portal game files"
    )
    parser.add_argument(
        "--extra-translations-dir", required=True,
        help="Directory containing Portal64-specific string files"
    )
    parser.add_argument(
        "--output-dir", required=True,
        help="Directory to write generated code"
    )
    parser.add_argument(
        "--languages", default="all", type=language_list,
        help="Comma-separated list of languages to include, or 'all'"
    )
    parser.add_argument(
        "--check-font",
        help="Provide font JSON file to output information about used characters"
    )

    return parser.parse_args()

args = get_args()
language_strings = parse_all_languages(
    args.game_root_dir,
    args.extra_translations_dir,
    args.languages
)

font_data = args.check_font
if font_data:
    check_font(language_strings, font_data)

output_dir = args.output_dir
write_string(
    os.path.join(output_dir, "strings.h"),
    generate_header(language_strings)
)
write_string(
    os.path.join(output_dir, "strings.c"),
    generate_main_source_file(language_strings)
)

default_strings = language_strings[list(language_strings)[0]]
for language, strings in language_strings.items():
    write_string(
        os.path.join(output_dir, f"strings_{language}.c"),
        generate_language_source_file(language, strings, default_strings)
    )
