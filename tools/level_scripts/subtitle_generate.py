#!/usr/bin/env python3
import os
import re
import sys
import json

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
        keyval= line.split('"\t"')
        if len(keyval) != 2:
            keyval= line.split('"   "')
            if len(keyval) != 2:
                continue
        if "[english]" in keyval[0]:
            continue
        key = keyval[0].replace('"', "").replace(".", "_").replace("-", "_").replace('\\', "_").replace('#', "").upper()
        val = keyval[1].replace('"', "").replace("\n", "").replace("\\", "")
        val = re.sub(r'\<clr.+\>','',val)
        val = re.sub(r'\<norepeat.+\>','',val)
        val = val.replace("<sfx>", "")
        val = re.sub(r'\<len:.+\>','',val)
        val = val.replace("<len>", "")
        newval = ""
        last_space = 0
        addition = 0
        for i,ch in enumerate(val):
            if (i%38 == 0) and (i != 0):
                newval = newval[:last_space+addition] + '\\n' + newval[last_space+addition+1:]
                addition += 1
                newval = newval + ch
            else:
                if ch == " ":
                    last_space = i
                newval = newval + ch
        keys.append(key)
        values.append(newval)
    
    return keys, values, language

def make_overall_subtitles_header(all_header_lines, languages_list):
    header_lines = []
    header_lines.append("#ifndef __SUBTITLES_H__\n")
    header_lines.append("#define __SUBTITLES_H__\n")
    header_lines.append("\n")
    header_lines.append(f"#define NUM_SUBTITLE_LANGUAGES {len(languages_list)}\n")
    header_lines.append(f"#define NUM_SUBTITLE_MESSAGES 508\n")
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

def make_subtitle_for_language(lang_lines, lang_name):
    lines = []

    idx = 1

    lines.append("\n")

    for value in lang_lines:
        lines.append(f'char __translation_{lang_name}_{idx}[] = "{value}";\n')
        idx = idx + 1

    lines.append("\n")
    lines.append(f"char* gSubtitle{lang_name}[508] = {'{'}\n")

    # SubtitleKeyNone
    lines.append('    "",\n')

    idx = 1

    for value in lang_lines:
        lines.append(f'    __translation_{lang_name}_{idx},\n')
        idx = idx + 1

    lines.append("};\n")

    dump_lines(f"build/src/audio/subtitles_{lang_name}.c", lines)

def determine_invalid_characters(lang_name, lang_lines, good_characters):
    used_characters = set()

    for value in lang_lines:
        used_characters = used_characters | set(value)

    invalid = used_characters - good_characters

    if len(invalid) == 0:
        return
    
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
            sourcefile_lines.append(f'    "{language.upper()}",\n')
    sourcefile_lines.append("};\n")
    sourcefile_lines.append("\n")

    for language in language_list:
        sourcefile_lines.append(f"extern char _subtitles_{language}SegmentRomStart[];\n")
        sourcefile_lines.append(f"extern char _subtitles_{language}SegmentRomEnd[];\n")
        sourcefile_lines.append(f"extern char* gSubtitle{language}[508];\n")
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

def process_all_closecaption_files(dir, language_names):
    values_list = []
    header_lines = []
    language_list = []
    language_with_values_list = []
    SubtitleKey_generated = False

    for langauge_name in language_names:
        filename = f"closecaption_{langauge_name}.txt"

        try:  
            filepath = os.path.join(dir, filename)
            lines = []
            
            with open(filepath, "r", encoding='utf-16-le') as f:
                lines = f.readlines()
            
            new_lines = []
            for line in lines:
                line = line.replace("\x00", "")
                if "\n" != line:
                    new_lines.append(line)

            k,v,l = get_caption_keys_values_language(new_lines)
            values_list.append(v)
            if not SubtitleKey_generated:
                header_lines = make_SubtitleKey_headerlines(k)
                SubtitleKey_generated = True
            language_list.append(l)
            
            language_with_values_list.append({
                'value': v,
                'name': l,
            })
            print(filename, " - PASSED")
        except Exception as e:
            print(e)
            print(filename, " - FAILED")
            continue

    good_characters = get_supported_characters()
    used_characters = set()

    for language in language_with_values_list:
        make_subtitle_for_language(language['value'], language['name'])
        used_characters = used_characters | determine_invalid_characters(language['name'], language['value'], good_characters)

    print(f"needed characters\n{''.join(sorted(list(good_characters & used_characters)))}")
    print(f"unused characters\n{''.join(sorted(list(good_characters - used_characters)))}")
    print(f"invalid characters\n{''.join(sorted(list(used_characters - good_characters)))}")

    make_subtitle_ld(language_with_values_list)

    make_overall_subtitles_header(header_lines, language_list)
    make_overall_subtitles_sourcefile(language_list)



process_all_closecaption_files("vpk/Portal/portal/resource", sys.argv[1:])