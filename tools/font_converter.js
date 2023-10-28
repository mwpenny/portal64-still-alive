// this tool takes a the json output from https://github.com/andryblack/fontbuilder and create a font usabe in portal64

// usage
// generate multiple font files with each image not being larger than 4kb
// then name the files font_file_0.json, font_file_1.json, font_file_2.json
// you also need a json file with all the characters in a single file called font_file_all.json
// this all file is only needed to extract the kerning
// once you have that use this as follows
//
// node tools/font_converter.js FontName /path/to/font_file /path/to/output.c

const fs = require('fs');

const name = process.argv[2];
const filePrefix = process.argv[3];

const joinedSymbols = [];
let index = 0;

while (index < 100) {
    const filename = `${filePrefix}_${index}.json`;
    if (!fs.existsSync(filename)) {
        break;
    }

    const singleInput = JSON.parse(fs.readFileSync(filename));

    singleInput.symbols.forEach((symbol) => {
        joinedSymbols.push({
            ...symbol,
            textureIndex: index,
        });
    });

    ++index;
}

const allSymbols = JSON.parse(fs.readFileSync(`${filePrefix}_all.json`));

const input = {
    kerning: allSymbols.kerning,
    config: allSymbols.config,
    symbols: joinedSymbols,
};

function kerningHashFunction(kerning, multiplier, arraySize) {
    return ((kerning.first * multiplier) + kerning.second) % arraySize;
}

function symbolHashFunction(symbol, multiplier, arraySize) {
    return (symbol.id * multiplier) % arraySize;
}

function checkForCollisions(list, hashFunction, multiplier, arraySize, emptyObj) {
    const sparseArray = [];
    sparseArray.length = arraySize;

    if (arraySize < list.length) {
        return null;
    }

    let maxCollisions = 0;
    let averageCollisions = 0;

    for (const element of list) {
        let index = hashFunction(element, multiplier, arraySize);

        let currentCollisions = 0;

        while (sparseArray[index]) {
            index = (index + 1) % arraySize;
            currentCollisions = currentCollisions + 1;
        }

        maxCollisions = Math.max(maxCollisions, currentCollisions);

        averageCollisions += currentCollisions;

        sparseArray[index] = element;
    }

    averageCollisions /= list.length;

    for (let i = 0; i < arraySize; ++i) {
        if (!sparseArray[i]) {
            sparseArray[i] = emptyObj;
        }
    }

    return {sparseArray, maxCollisions, averageCollisions};
}

function searchForBestHashTable(list, hashFunction, emptyObj) {
    let result;
    let multiplier;

    let arraySize = 1;
    let mask = 1;

    while (arraySize < list.length) {
        arraySize *= 2;
        mask <<= 1;
    }

    arraySize *= 2;
    mask <<= 1;

    for (let i = 1; i < 0x10000; ++i) {
        const check = checkForCollisions(list, hashFunction, i, arraySize, emptyObj);

        if (!result || check.maxCollisions < result.maxCollisions) {
            result = check
            multiplier = i;
        }
    }

    mask -= 1;

    return {result: result.sparseArray, multiplier: multiplier, mask: mask, maxCollisions: result.maxCollisions, averageCollisions: result.averageCollisions};
}

function buildKerning(kerningList) {
    return `struct FontKerning g${name}Kerning[] = {
${kerningList.map(kerning => `    {.amount = ${kerning.amount}, .first = ${kerning.first}, .second = ${kerning.second}},`).join('\n')}
};
`
}

function buildFont(kerningResult, symbolResult) {
    return `struct Font g${name}Font = {
    .kerning = &g${name}Kerning[0],
    .symbols = &g${name}Symbols[0],
    .base = ${input.config.base},
    .charHeight = ${input.config.charHeight},
    .symbolMultiplier = ${symbolResult.multiplier},
    .symbolMask = 0x${symbolResult.mask.toString(16)},
    .symbolMaxCollisions = ${symbolResult.maxCollisions},
    .kerningMultiplier = ${kerningResult.multiplier},
    .kerningMask = 0x${kerningResult.mask.toString(16)},
    .kerningMaxCollisions = ${kerningResult.maxCollisions},
};
`
}

function buildSymbol(symbol) {
    return `    {
        .id = ${symbol.id},
        .x = ${symbol.x}, .y = ${symbol.y},
        .width = ${symbol.width}, .height = ${symbol.height},
        .xoffset = ${symbol.xoffset}, .yoffset = ${symbol.yoffset},
        .xadvance = ${symbol.xadvance},
        .textureIndex = ${symbol.textureIndex},
    },`
}

const kerningResult = searchForBestHashTable(
    input.kerning, 
    kerningHashFunction, 
    {amount: 0, first: 0, second: 0}
);

const symbolResult = searchForBestHashTable(
    input.symbols, 
    symbolHashFunction, 
    {id: 0, x: 0, y: 0, width: 0, height: 0, xoffset: 0, yoffset: 0, xadvance: 0, textureIndex: -1}
);

console.log(`symbolLength = ${input.symbols.length}/${symbolResult.result.length}`);
console.log(`symbolMaxCollisions = ${symbolResult.maxCollisions}`);
console.log(`symbolAverageCollisions = ${symbolResult.averageCollisions}`);
console.log(`kerningLength = ${input.kerning.length}/${kerningResult.result.length}`);
console.log(`maxKerningCollisions = ${kerningResult.maxCollisions}`);
console.log(`kerningAverageCollisions = ${kerningResult.averageCollisions}`);

fs.writeFileSync(process.argv[4], `

#include "font.h"

${buildKerning(kerningResult.result)}

struct FontSymbol g${name}Symbols[] = {
${symbolResult.result.map(buildSymbol).join('\n')}
};

${buildFont(kerningResult, symbolResult)}
`);