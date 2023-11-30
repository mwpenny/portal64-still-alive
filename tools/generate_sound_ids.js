
const fs = require('fs');
const path = require('path');

let output = '';
let inputs = [];
let definePrefix = '';
let lastCommand = '';

let outputLanguagesSourceFile = '';
let outputLanguagesHeader = '';
let allSounds = [];
let languages = [];
let languages_codes = ["EN", "DE", "ES", "FR", "RU"];
let language_names = {"EN": "English", "DE": "Deutsch", "ES": "Español", "FR": "Français",  "RU": "Русский язык"};
let lookup = [];
languages.push("EN"); // Always included by default

for (let i = 2; i < process.argv.length; ++i) {
    const arg = process.argv[i];
    if (lastCommand) {
        if (lastCommand == '-o') {
            output = arg;
        } else if (lastCommand == '-p') {
            definePrefix = arg;
        }
        lastCommand = '';
    } else if (arg[0] == '-') {
        lastCommand = arg;
    } else {
        inputs.push(arg);
    }
}

outputLanguagesSourceFile = output + '/languages.c';
outputLanguagesHeader += output + '/languages.h';
output += '/clips.h';

inputs.push('TOTAL_COUNT');

const invalidCharactersRegex = /[^\w\d_]+/gm;

function formatSoundName(soundFilename, index) {
    const extension = path.extname(soundFilename);
    const lastPart = path.basename(soundFilename, extension);
    const defineName = definePrefix + lastPart.replace(invalidCharactersRegex, '_').toUpperCase();
    
    trackSoundLanguages(soundFilename, defineName, index);
    return `#define ${defineName} ${index}`;
}

function formatFile(outputFilename, soundFilenames) {
    const defineName = outputFilename.replace(invalidCharactersRegex, '_').toUpperCase();
    return `#ifndef ${defineName}
#define ${defineName}

${soundFilenames.map(formatSoundName).join('\n')}

#endif`
}

fs.writeFileSync(output, formatFile(output, inputs));

function trackSoundLanguages(soundFilename, defineName, index) {
    languages_codes.forEach((language) => {
        if (defineName.includes("_" + language + "_") && !languages.includes(language))
            languages.push(language);
    });
    if (soundFilename != "TOTAL_COUNT")
        allSounds.push({defineName: defineName, index: index});
}

function fillOverrideLookup() {
    for (let language of languages) {
        for (let overrideSound of allSounds) {
            let baseSound = overrideSound;
            let overrideName = overrideSound.defineName;
            // check if current sound has base version of default language
            if (overrideName.includes('_' + language + '_') && language != "EN") {
                baseSound = allSounds.find(element => element.defineName == overrideName.replace('_' + language + '_', '_'));
            }
            lookup.push({language: language, baseIndex: baseSound.index, index: overrideSound.index, defineName: overrideSound.defineName});
        }
    }
}

function generateLanguagesHeader() {

    let header = `#ifndef __LANGUAGES_H__
#define __LANGUAGES_H__

#define NUM_AUDIO_LANGUAGES ${languages.length}

extern char* AudioLanguages[];
extern int AudioLanguageValues[][${allSounds.length}];

`
    header += 'enum AudioLanguagesKey\n{\n';
    header += (languages.length > 0 ? '\tAUDIO_LANGUAGE_' + languages.join(',\n\tAUDIO_LANGUAGE_') + ',\n' : '');
    header += '};\n';
    header += '#endif';
    
    return header;
}

function generateLanguagesSourceFile() {
    fillOverrideLookup();
    let sourcefile = '#include "languages.h"\n';
    sourcefile += '\n';
    
    sourcefile += 'char* AudioLanguages[] = \n{\n';
    
    for (let language of languages) {
        sourcefile += '\t"' + language_names[language] + '",\n';
    }

    sourcefile += '};\n';
    
    sourcefile += 'int AudioLanguageValues[][' + allSounds.length + '] = \n{\n';

    for (let language of languages) {
        if (language == "EN") continue; // save RAM
        sourcefile += '\t//' + language + '\n\t{\n';

        for (let baseSound of allSounds) {
            let overrideSound = lookup.find(lookElement => lookElement.language == language && lookElement.baseIndex == baseSound.index && lookElement.baseIndex != lookElement.index);
            if (overrideSound === undefined) overrideSound = baseSound; // no override, use default
            sourcefile += '\t' + overrideSound.index + ', // ' + baseSound.defineName + ' ('+baseSound.index+') -> ' + overrideSound.defineName + ' ('+ overrideSound.index+')' + '\n';
        }
        sourcefile += '\t},\n';
    }
    sourcefile += '};';
    return sourcefile;
}

// sort found languages in order of elements in language_codes
let temp = [...languages_codes];
languages = temp.filter(function(cItem) {
  return languages.find(function(aItem) {
    return cItem === aItem
  })
})

fs.writeFileSync(outputLanguagesHeader, generateLanguagesHeader());
fs.writeFileSync(outputLanguagesSourceFile, generateLanguagesSourceFile());
