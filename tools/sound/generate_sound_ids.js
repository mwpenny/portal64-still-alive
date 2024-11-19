const fs = require('fs');
const path = require('path');
const util = require('util')

const SUPPORTED_LANGUAGES = {
    "english": "English",
    "french":  "Français",
    "german":  "Deutsch",
    "russian": "Русский язык",
    "spanish": "Español"
};

const INVALID_TOKEN_CHARACTER = /[^A-Za-z0-9_]/gim;

// Helper functions

function sanitize(s) {
    return s.replace(INVALID_TOKEN_CHARACTER, '_');
}

function getSoundName(soundFile, stripPrefix) {
    let { name } = path.parse(soundFile);

    if (stripPrefix && name.startsWith(stripPrefix)) {
        name = name.substring(stripPrefix.length);
    }

    return `SOUNDS_${sanitize(name).toUpperCase()}`;
}

function getSoundLanguage(soundFile) {
    const { name } = path.parse(soundFile);

    return Object.keys(SUPPORTED_LANGUAGES).find(lang => {
        return name.toLowerCase().startsWith(`${lang}_`);
    });
}

function getSoundNameWithoutLanguage(soundFile, language) {
    return getSoundName(soundFile, language && `${language}_`);
}

function getProvidedLanguages(soundFiles) {
    const languageNames = new Set();

    for (const soundFile of soundFiles) {
        const language = getSoundLanguage(soundFile);
        if (language) {
            languageNames.add(language);
        }
    }

    // Sort in consistent order
    return Object.keys(SUPPORTED_LANGUAGES)
        .filter(lang => languageNames.has(lang));
}

function validateLocalizedSounds(languages) {
    for (const [language, localizedSounds] of languages) {
        let prevIndex;
        for (const sound of localizedSounds.values()) {
            if (prevIndex !== undefined && sound.index != prevIndex + 1) {
                throw new Error(
                    `Localized sound indices must be contiguous. ` +
                    `Found invalid index for language '${language}' and sound '${sound.localizedName}' (${sound.index})`
                );
            }
            prevIndex = sound.index;
        }
    }
}

function parseSounds(soundFiles) {
    const languageNames = getProvidedLanguages(soundFiles);
    const defaultLanguage = languageNames[0];

    // Initialize in language order so output order is consistent
    // ES6 maps guarantee iteration and insertion order are the same
    const languages = new Map();
    for (const languageName of languageNames) {
        languages.set(languageName, new Map());
    }

    const unlocalized = new Map();

    for (const [i, soundFile] of soundFiles.entries()) {
        const language = getSoundLanguage(soundFile);
        const canonicalName = getSoundNameWithoutLanguage(soundFile, language);
        const sound = {
            // The game references sounds by canonical name
            // Remove language prefix on default language sounds
            localizedName: getSoundNameWithoutLanguage(soundFile, defaultLanguage),
            index: i
        };

        const soundMap = language ? languages.get(language) : unlocalized;
        soundMap.set(canonicalName, sound);
    }

    validateLocalizedSounds(languages);
    return { languages, unlocalized, defaultLanguage };
}

// clips.h

function generateSoundIndices(soundInfo) {
    const allSounds = [
        ...soundInfo.unlocalized.values(),
        ...[...soundInfo.languages.values()]
            .map(sounds => [...sounds.values()])
            .flat()
    ];

    return allSounds.map(s => `#define ${s.localizedName} ${s.index}`);
}

function generateClipsHeaderFile(soundInfo) {
    const soundIndices = generateSoundIndices(soundInfo);

    return `#ifndef __SOUND_CLIPS_H__
#define __SOUND_CLIPS_H__

${soundIndices.join('\n')}
#define SOUNDS_TOTAL_COUNT ${soundIndices.length}

#endif`;
}

// languages.h

function generateLanguageEnumEntries(languages) {
    return [...languages.keys()].map(k => {
        return `\tAUDIO_LANGUAGE_${k.toUpperCase()}`;
    });
}

function generateLanguagesHeaderFile(soundInfo) {
    const { languages, defaultLanguage } = soundInfo;
    const defaultLanguageSounds = languages.get(defaultLanguage);

    return `#ifndef __LANGUAGES_H__
#define __LANGUAGES_H__

#define NUM_AUDIO_LANGUAGES ${languages.size}
#define FIRST_LOCALIZED_SOUND ${[...defaultLanguageSounds.values()][0].index}
#define NUM_LOCALIZED_SOUNDS ${defaultLanguageSounds.size}

extern char* AudioLanguages[];
extern int AudioLanguageValues[][NUM_LOCALIZED_SOUNDS];

enum AudioLanguagesKey
{
${generateLanguageEnumEntries(languages).join(',\n')}
};

#endif`;
}

// languages.c

function generateLanguageNameStrings(languages) {
    return [...languages.keys()]
        .map(lang => `\t"${SUPPORTED_LANGUAGES[lang]}"`);
}

function generateLanguageSoundEntries(soundInfo) {
    const { languages, defaultLanguage } = soundInfo;
    const defaultLanguageSounds = languages.get(defaultLanguage);

    const entries = [];

    for (const [language, localizedSounds] of languages.entries()) {
        if (language === defaultLanguage) {
            // Default language sounds are played directly without using the table
            continue;
        }

        entries.push(`\t// ${language}`);
        entries.push('\t{');

        // Try to use localized sounds, fall back to default language
        for (const [name, sound] of defaultLanguageSounds.entries()) {
            const soundToUse = localizedSounds.get(name) || sound;

            let entry = `\t\t${soundToUse.index},`;
            entry += `  // ${sound.localizedName} (${sound.index}) -> `;
            entry += `${soundToUse.localizedName} (${soundToUse.index})`;

            entries.push(entry);
        }

        entries.push('\t},');
    }

    return entries;
}

function generateLanguagesSourceFile(soundInfo) {
    return `#include "languages.h"

char* AudioLanguages[] = {
${generateLanguageNameStrings(soundInfo.languages).join(',\n')}
};

int AudioLanguageValues[][NUM_LOCALIZED_SOUNDS] = {
${generateLanguageSoundEntries(soundInfo).join('\n')}
};`;
}

// Main
const { values, positionals } = util.parseArgs({
    options: {
        'out-dir': {
            type: 'string'
        }
    },
    allowPositionals: true
});

const outDir = values['out-dir'];
const soundFiles = positionals;

if (!fs.existsSync(outDir)) {
    fs.mkdirSync(outDir, { recursive: true });
}

const soundInfo = parseSounds(soundFiles);

fs.writeFileSync(
    path.join(outDir, 'clips.h'),
    generateClipsHeaderFile(soundInfo)
);
fs.writeFileSync(
    path.join(outDir, 'languages.h'),
    generateLanguagesHeaderFile(soundInfo)
);
fs.writeFileSync(
    path.join(outDir, 'languages.c'),
    generateLanguagesSourceFile(soundInfo)
);
