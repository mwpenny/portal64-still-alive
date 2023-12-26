#!/usr/bin/env python3
import os
import re
import sys
import json
from os.path import exists

hl_gameui_whitelist = {
    "GAMEUI_ASPECTWIDE",
    "GAMEUI_AUDIO",
    "GAMEUI_AUTOSAVE",
    "GAMEUI_CANCEL",
    "GAMEUI_CAPTIONING",
    "GAMEUI_CHAPTER",
    "GAMEUI_CONFIRMDELETESAVEGAME_INFO",
    "GAMEUI_CONFIRMDELETESAVEGAME_OK",
    "GAMEUI_CONFIRMDELETESAVEGAME_TITLE",
    "GAMEUI_CONFIRMOVERWRITESAVEGAME_INFO",
    "GAMEUI_CONFIRMOVERWRITESAVEGAME_OK",
    "GAMEUI_CONFIRMOVERWRITESAVEGAME_TITLE",
    "GAMEUI_DELETE",
    "GAMEUI_GAMEMENU_QUIT",
    "GAMEUI_GAMEMENU_RESUMEGAME",
    "GAMEUI_JOYSTICKINVERTED",
    "GAMEUI_LOAD",
    "GAMEUI_LOADGAME",
    "GAMEUI_MUSICVOLUME",
    "GAMEUI_NEWGAME",
    "GAMEUI_NEWSAVEGAME",
    "GAMEUI_OPTIONS",
    "GAMEUI_PLAYERNAME",
    "GAMEUI_PORTAL",
    "GAMEUI_PORTALDEPTHLABEL",
    "GAMEUI_PORTALFUNNEL",
    "GAMEUI_SAVE",
    "GAMEUI_SAVEGAME",
    "GAMEUI_SOUNDEFFECTVOLUME",
    "GAMEUI_SUBTITLES",
    "GAMEUI_SUBTITLESANDSOUNDEFFECTS",
    "GAMEUI_USEDEFAULTS",
    "GAMEUI_VIDEO",
}

portal_whitelist = {
    "PORTAL_CHAPTER1_TITLE",
    "VALVE_PRIMARY_ATTACK",
    "VALVE_SECONDARY_ATTACK",
    "HINT_GET_PORTAL_1",
    "HINT_GET_PORTAL_2",
    "HINT_USE_ITEMS",
    "HINT_DROP_ITEMS",
    "HINT_USE_SWITCHES",
    "HINT_DUCK",
    "HINT_JUMP",
}

valve_whitelist = {
    "VALVE_JUMP",
    "VALVE_DUCK",
    "VALVE_USE_ITEMS",
    "VALVE_COMBAT_TITLE",
    "VALVE_MOVEMENT_TITLE",
    "VALVE_MISCELLANEOUS_TITLE",
    "VALVE_PAUSE_GAME",
    "VALVE_LOOK_STRAIGHT_AHEAD",
    "VALVE_MISCELLANEOUS_KEYBOARD_KEYS_TITLE",
}

#include only languages the font supports (ingame lists display in this defined order)
language_translations = {	
    'english': 'English',
    'brazilian': 'Brasileiro',
    'bulgarian': 'Български език',
    'czech': 'Čeština',
    'danish': 'Dansk',
    'german': 'Deutsch',
    'spanish': 'Español',
    'latam': 'Español americana',
    'greek': 'Ελληνικά',
    'french': 'Français',
    'italian': 'Italiano',
    'polish': 'Język polski',
    'hungarian': 'Magyar nyelv',
    'dutch': 'Nederlands',
    'norwegian': 'Norsk',
    'portuguese': 'Português',
    'russian': 'Русский язык',
    'romanian': 'Românește',
    'finnish': 'Suomi',
    'swedish': 'Svenska',
    'turkish': 'Türkçe',
    'ukrainian': 'Українська мова',
}

def get_supported_characters():
    with open('assets/fonts/dejavu_sans_book_8.json', 'r') as f:
        content = json.loads('\n'.join(f.readlines()))

    result = {' ', '\t', '\n', '\r'}
    
    for symbol in content['symbols']:
        result.update(chr(symbol['id']))

    return result


def dump_lines(sourcefile_path, lines):
    if not os.path.exists(os.path.dirname(os.path.abspath(sourcefile_path))):
        os.makedirs(os.path.dirname(os.path.abspath(sourcefile_path)))

    with open(sourcefile_path, "w") as f:
        f.writelines(lines)

def get_caption_keys_values_language(lines):
    language = "English"
    keys = []
    values = []

    found_language = False
    found_tokens = False
    for line in lines:
        if not found_language:
            if not '"Language"' in line:
                continue
            found_language = True
            line = line.replace('"', "")
            key, val = line.split()
            language = val
            continue
        if not found_tokens:
            if not '"Tokens"' in line:
                continue
            found_tokens = True
            continue
        if ("{" in line) or ("}" in line) :
            continue
        keyval= re.split('"\\s+"', line)
        if len(keyval) != 2:
            keyval= line.split('"   "')
            if len(keyval) != 2:
                continue

        if "[$X360]" in keyval[1]:
            continue
        if "[english]" in keyval[0] or 'commentary' in keyval[0]:
            continue

        if "[$WIN32]" in keyval[1]:
            keyval[1] = keyval[1].replace("[$WIN32]", "")

        key = keyval[0].replace('"', "").replace(".", "_").replace("-", "_").replace('\\', "_").replace('#', "").upper()
        val = keyval[1].replace('"', "").replace("\n", "").replace("\\n", " ").replace("\\", "").strip()
        val = re.sub(r'\<clr.+\>','',val)
        val = re.sub(r'\<norepeat.+\>','',val)
        val = val.replace("<sfx>", "")
        val = re.sub(r'\<len:.+\>','',val)
        val = val.replace("<len>", "")
        keys.append(key)
        values.append(val)
    
    return keys, values, language

def make_overall_subtitles_header(all_header_lines, languages_list, message_count, max_message_length):
    header_lines = []
    header_lines.append("#ifndef __SUBTITLES_H__\n")
    header_lines.append("#define __SUBTITLES_H__\n")
    header_lines.append("\n")
    header_lines.append(f"#define NUM_SUBTITLE_LANGUAGES {len(languages_list)}\n")
    header_lines.append(f"#define NUM_SUBTITLE_MESSAGES {message_count + 1}\n")
    header_lines.append(f"#define MAX_SUBTITLE_LENGTH  {max_message_length}\n")
    header_lines.append("\n")
    header_lines.append("struct SubtitleBlock {\n")
    header_lines.append("    char* romStart;\n")
    header_lines.append("    char* romEnd;\n")
    header_lines.append("    char** values;\n")
    header_lines.append("};\n")
    header_lines.append("\n")
    header_lines.append("extern char* SubtitleLanguages[];\n")
    header_lines.append("extern struct SubtitleBlock SubtitleLanguageBlocks[];\n")
    header_lines.append("\n")

    for idx, language in enumerate(languages_list):
        header_lines.append(f"#define LANGUAGE_{language.upper()} {idx}\n")

    header_lines.append("\n")


    if len(languages_list) > 0:
        header_lines.extend(all_header_lines)
    else:
        header_lines.append(f"enum SubtitleKey\n")
        header_lines.append("{\n")
        header_lines.append('    SubtitleKeyNone,\n')
        header_lines.append("};\n")
        header_lines.append("\n")

    header_lines.append("#endif")

    dump_lines("build/src/audio/subtitles.h", header_lines)

def make_SubtitleKey_headerlines(keys):
    header_lines = []
    header_lines.append("\n")
    header_lines.append(f"enum SubtitleKey\n")
    header_lines.append("{\n")
    header_lines.append('    SubtitleKeyNone,\n')
    for key in keys:
        header_lines.append(f'    {key},\n')
    header_lines.append("};\n")
    header_lines.append("\n")
    return header_lines

def make_subtitle_for_language(lang_lines, lang_name, keys):
    lines = []

    lines.append('#include "subtitles.h"')
    lines.append("\n")
    lines.append("\n")

    for idx, value in enumerate(lang_lines):
        lines.append(f'char __translation_{lang_name}_{keys[idx]}[] = "{value}";\n')

    lines.append("\n")
    lines.append(f"char* gSubtitle{lang_name}[NUM_SUBTITLE_MESSAGES] = {'{'}\n")

    # SubtitleKeyNone
    lines.append('    "",\n')

    for idx, value in enumerate(lang_lines):
        lines.append(f'    __translation_{lang_name}_{keys[idx]},\n')

    lines.append("};\n")

    dump_lines(f"build/src/audio/subtitles_{lang_name}.c", lines)

def determine_invalid_characters(lang_name, lang_lines, good_characters):
    used_characters = set()

    for value in lang_lines:
        used_characters = used_characters | set(value)

    invalid = used_characters - good_characters

    if len(invalid) == 0:
        return used_characters
    
    print(f"{lang_name} has {len(invalid)} invalid charcters\n{''.join(sorted(list(invalid)))}")

    return used_characters


def make_subtitle_ld(languages):
    lines = []

    for language in languages:
        language_name = language['name']

        lines.append(f"    __romPos = (__romPos + 15) & ~0xF;\n")
        lines.append(f"    BEGIN_SEG(subtitles_{language_name}, 0x04000000)\n")
        lines.append("    {\n")
        lines.append(f"       build/src/audio/subtitles_{language_name}.o(.data);\n")
        lines.append(f"       build/src/audio/subtitles_{language_name}.o(.bss);\n")
        lines.append("    }\n")
        lines.append(f"    END_SEG(subtitles_{language_name})\n")
        lines.append("\n")

    dump_lines('build/subtitles.ld', lines)

def make_overall_subtitles_sourcefile(language_list):
    sourcefile_lines = []
    sourcefile_lines.append('#include "subtitles.h"\n')

    sourcefile_lines.append("\n")
    sourcefile_lines.append("char* SubtitleLanguages[] =\n")
    sourcefile_lines.append("{\n")
    if len(language_list) <= 0:
        sourcefile_lines.append(f'    "",\n')
    else:
        for language in language_list:
            sourcefile_lines.append(f'    "{language_translations[language]}",\n')
    sourcefile_lines.append("};\n")
    sourcefile_lines.append("\n")

    for language in language_list:
        sourcefile_lines.append(f"extern char _subtitles_{language}SegmentRomStart[];\n")
        sourcefile_lines.append(f"extern char _subtitles_{language}SegmentRomEnd[];\n")
        sourcefile_lines.append(f"extern char* gSubtitle{language}[NUM_SUBTITLE_MESSAGES];\n")
        sourcefile_lines.append("\n")

    sourcefile_lines.append("struct SubtitleBlock SubtitleLanguageBlocks[] = {\n")

    for language in language_list:
        sourcefile_lines.append("    {\n")
        sourcefile_lines.append(f"        _subtitles_{language}SegmentRomStart,\n")
        sourcefile_lines.append(f"        _subtitles_{language}SegmentRomEnd,\n")
        sourcefile_lines.append(f"        gSubtitle{language},\n")
        sourcefile_lines.append("    },\n")

    sourcefile_lines.append("};\n")
    sourcefile_lines.append("\n")

    dump_lines("build/src/audio/subtitles.c", sourcefile_lines)

def read_translation_file(filepath, encoding='utf-16-le'):
    if not exists(filepath):
        raise Exception(f"not found {filepath}")

    lines = []
    
    with open(filepath, "r", encoding=encoding) as f:
        lines = f.readlines()
    
    new_lines = []
    for line in lines:
        line = line.replace("\x00", "")
        if "\n" != line:
            new_lines.append(line)

    return get_caption_keys_values_language(new_lines)

def filter_whitelist(keys, values, whitelist):
    result_keys = []
    result = []

    for index in range(len(keys)):
        if keys[index] in whitelist:
            result_keys.append(keys[index])
            value = values[index]
            if "HINT_" in keys[index]:
                value = re.sub('%[^%]*%', '', value).strip()
            result.append(value)

    return result_keys, result

def process_all_closecaption_files(dir, language_names):
    values_list = []
    header_lines = []
    language_list = []
    language_with_values_list = []
    key_order = None
    default_values = None

    for language_name in language_names:
        filename = f"closecaption_{language_name}.txt"

        filepath = os.path.join(dir, filename)

        k,v,l = read_translation_file(filepath)

        gamepad_k, gamepad_v, _ = read_translation_file(f"vpk/Portal/hl2/resource/gameui_{language_name}.txt")
        gamepad_k, gamepad_v = filter_whitelist(gamepad_k, gamepad_v, hl_gameui_whitelist)

        portal_k, portal_v, _ = read_translation_file(f"vpk/Portal/portal/resource/portal_{language_name}.txt")
        portal_k, portal_v = filter_whitelist(portal_k, portal_v, portal_whitelist)

        valve_k, valve_v, _ = read_translation_file(f"vpk/Portal/hl2/resource/valve_{language_name}.txt")
        valve_k, valve_v = filter_whitelist(valve_k, valve_v, valve_whitelist)

        extra_k, extra_v, _ = read_translation_file(f"assets/translations/extra_{language_name}.txt", encoding='utf-8')

        k = k + gamepad_k + portal_k + valve_k + extra_k
        v = v + gamepad_v + portal_v + valve_v + extra_v

        if not key_order:
            header_lines = make_SubtitleKey_headerlines(k)
            key_order = k
            default_values = v
        else:
            index_mapping = {}
            for idx, x in enumerate(k):
                index_mapping[x] = idx

            new_values = []

            for idx, key in enumerate(key_order):
                if key in index_mapping:
                    new_values.append(v[index_mapping[key]])
                else:
                    new_values.append(default_values[idx])

            v = new_values

        values_list.append(v)
        language_list.append(l)
        
        language_with_values_list.append({
            'value': v,
            'name': l,
        })
        print(filename, " - PASSED")

    good_characters = get_supported_characters()
    used_characters = set()

    max_message_length = 0

    for language in language_with_values_list:
        make_subtitle_for_language(language['value'], language['name'], key_order)
        used_characters = used_characters | determine_invalid_characters(language['name'], language['value'], good_characters)

        for value in language['value']:
            max_message_length = max(max_message_length, len(value))

    print(f"needed characters\n{''.join(sorted(list(good_characters & used_characters)))}")
    print(f"unused characters\n{''.join(sorted(list(good_characters - used_characters)))}")
    print(f"invalid characters\n{''.join(sorted(list(used_characters - good_characters)))}")
    print(f"max message length\n{max_message_length}")

    make_subtitle_ld(language_with_values_list)

    make_overall_subtitles_header(header_lines, language_list, len(language_with_values_list[0]['value']), max_message_length)
    make_overall_subtitles_sourcefile(language_list)

dir = "vpk/Portal/portal/resource"

#actually available supported languages
available_languages_list = []
ordered_language_list = []

lst = os.listdir(dir)
lst.sort()
for filename in lst:
    for language in language_translations:
        if language in filename:
            if language not in available_languages_list:
                available_languages_list.append(language)

for language in language_translations:
    if language in available_languages_list:
        ordered_language_list.append(language)

process_all_closecaption_files(dir, ordered_language_list)
