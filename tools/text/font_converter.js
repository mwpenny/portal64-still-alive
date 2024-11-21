// This tool takes a the json output from https://github.com/andryblack/fontbuilder
// and generates C code containing kerning and symbol information

// Usage:
// 1. Use fontbuilder to generate multiple font files
//  - Settings used by game fonts: layout=optimized box, padding=1px right
//  - Each image must fit in tmem (e.g., up to 128x64 when using 4 bits per texel)
//  - Name each JSON file <font>_0.json, <font>_1.json, ..., <font>_N.json
//  - Name the font directory the same as the font
//
// 2. Also generate a single font with all characters on the same line
//  - Name the JSON file <font>_all.json
//  - This file is only needed to extract the kerning
//
// 3. Use this tool as follows:
//  - node font_converter.js FontName /path/to/font_dir /path/to/output_dir

const fs = require('fs');
const path = require('path');

const INVALID_TOKEN_CHARACTER = /[^A-Za-z0-9_]/gim;

const MAX_HASH_MULTIPLIER = 0x10000;

function sanitize(s) {
    return s.replace(INVALID_TOKEN_CHARACTER, '_');
}

function loadFont(fontDir) {
    const dirName = path.basename(fontDir);
    const fontFileRegex = new RegExp(String.raw`${dirName}_(\d)+\.json`);

    const symbols = [];

    // Find all font page JSON files and load their symbols
    const fontPageFiles = [];
    for (let file of fs.readdirSync(fontDir)) {
        const match = file.match(fontFileRegex);

        if (match) {
            file = path.join(fontDir, file);
            fontPageFiles.push(file);

            const textureIndex = Number(match[1]);
            const fontData = JSON.parse(fs.readFileSync(file));

            symbols.push(
                ...fontData.symbols.map(s => ({
                    ...s,
                    textureIndex
                }))
            )
        }
    }

    // Load metadata and kerning from global file
    const fontMasterFile = fontPageFiles.length === 1 ?
        fontPageFiles[0] :
        path.join(fontDir, `${dirName}_all.json`);

    const { config, kerning } = JSON.parse(fs.readFileSync(fontMasterFile));
    return { config, kerning, symbols };
}

function kerningHashFunction(kerning, multiplier, tableSize) {
    return ((kerning.first * multiplier) + kerning.second) % tableSize;
}

function symbolHashFunction(symbol, multiplier, tableSize) {
    return (symbol.id * multiplier) % tableSize;
}

function buildHashTable(list, hashFunction, multiplier, tableSize, emptyObj) {
    const sparseArray = Array(tableSize);

    let maxCollisions = 0;
    let averageCollisions = 0;

    for (const element of list) {
        let currentCollisions = 0;
        let index = hashFunction(element, multiplier, tableSize);

        // Find the next available slot in the event of a collision
        // The array is sized such that there will always be a slot eventually
        while (sparseArray[index]) {
            index = (index + 1) % tableSize;
            ++currentCollisions;
        }

        maxCollisions = Math.max(maxCollisions, currentCollisions);
        averageCollisions += currentCollisions;

        sparseArray[index] = element;
    }

    if (list.length > 0) {
        averageCollisions /= list.length;
    }

    // Fill remaining slots
    for (let i = 0; i < tableSize; ++i) {
        if (!sparseArray[i]) {
            sparseArray[i] = emptyObj;
        }
    }

    return { sparseArray, maxCollisions, averageCollisions };
}

function searchForBestHashTable(list, hashFunction, emptyObj) {
    let bestHashTable;
    let multiplier;

    // Pad table to next-next power of 2
    // Reduces likelihood of collisions and allows easy wrapping with bitwise AND
    const exponent = list.length ? Math.ceil(Math.log2(list.length)) : 0;
    const tableSize = Math.pow(2, exponent + 1);
    const mask = tableSize - 1;

    for (let i = 1; i < MAX_HASH_MULTIPLIER; ++i) {
        const hashTable = buildHashTable(list, hashFunction, i, tableSize, emptyObj);

        if (!bestHashTable || hashTable.maxCollisions < bestHashTable.maxCollisions) {
            bestHashTable = hashTable
            multiplier = i;
        }
    }

    return { ...bestHashTable, multiplier, mask };
}

// Kerning

function generateKerning(kerning) {
    return `    {.amount = ${kerning.amount}, .first = ${kerning.first}, .second = ${kerning.second}},`;
}

function generateKerningTable(fontName, kerningList) {
    return `struct FontKerning g${fontName}Kerning[] = {
${kerningList.map(generateKerning).join('\n')}
};`
}

// Symbols

function generateSymbol(symbol) {
    return `    {
        .id = ${symbol.id},
        .x = ${symbol.x}, .y = ${symbol.y},
        .width = ${symbol.width}, .height = ${symbol.height},
        .xoffset = ${symbol.xoffset}, .yoffset = ${symbol.yoffset},
        .xadvance = ${symbol.xadvance},
        .textureIndex = ${symbol.textureIndex},
    },`
}

function generateSymbolTable(fontName) {
    return `struct FontSymbol g${fontName}Symbols[] = {
${symbolTable.sparseArray.map(generateSymbol).join('\n')}
};`
}

// Font

function generateFont(fontName, config, kerningTable, symbolTable) {
    return `struct Font g${fontName}Font = {
    .kerning = &g${fontName}Kerning[0],
    .symbols = &g${fontName}Symbols[0],
    .base = ${config.base},
    .charHeight = ${config.charHeight},
    .symbolMultiplier = ${symbolTable.multiplier},
    .symbolMask = 0x${symbolTable.mask.toString(16)},
    .symbolMaxCollisions = ${symbolTable.maxCollisions},
    .kerningMultiplier = ${kerningTable.multiplier},
    .kerningMask = 0x${kerningTable.mask.toString(16)},
    .kerningMaxCollisions = ${kerningTable.maxCollisions},
};`
}

function generateSourceFile(fontName, config, kerningTable, symbolTable) {
    fontName = sanitize(fontName);
    return `#include "font/font.h"

${generateKerningTable(fontName, kerningTable.sparseArray)}

${generateSymbolTable(fontName, symbolTable.sparseArray)}

${generateFont(fontName, config, kerningTable, symbolTable)}`;
}

// Main
const [fontName, fontDir, outputFile] = process.argv.slice(2);

const outputDir = path.dirname(outputFile);
if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
}

const { config, kerning, symbols } = loadFont(fontDir);

const kerningTable = searchForBestHashTable(
    kerning,
    kerningHashFunction,
    {
        amount: 0,
        first: 0,
        second: 0
    }
);

const symbolTable = searchForBestHashTable(
    symbols,
    symbolHashFunction,
    {
        id: 0,
        x: 0, y: 0,
        width: 0, height: 0,
        xoffset: 0, yoffset: 0,
        xadvance: 0,
        textureIndex: -1
    }
);

console.log(`symbolLength = ${symbols.length}/${symbolTable.sparseArray.length}`);
console.log(`symbolMaxCollisions = ${symbolTable.maxCollisions}`);
console.log(`symbolAverageCollisions = ${symbolTable.averageCollisions}`);
console.log(`kerningLength = ${kerning.length}/${kerningTable.sparseArray.length}`);
console.log(`maxKerningCollisions = ${kerningTable.maxCollisions}`);
console.log(`kerningAverageCollisions = ${kerningTable.averageCollisions}`);

fs.writeFileSync(
    outputFile,
    generateSourceFile(fontName, config, kerningTable, symbolTable)
);
