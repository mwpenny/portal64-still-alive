// this tool takes a the json output from https://github.com/andryblack/fontbuilder and create a font usabe in portal64

const fs = require('fs');

const name = process.argv[2];
const input = JSON.parse(fs.readFileSync(process.argv[3]));

function hashFunction(first, second, multiplier, arraySize) {
    return ((first * multiplier) + second) % arraySize;
}

function checkForCollisions(kerningList, multiplier, arraySize) {
    const sparseArray = [];
    sparseArray.length = arraySize;

    if (arraySize < kerningList.length) {
        return null;
    }

    let maxCollisions = 0;

    for (const kerning of kerningList) {
        let index = hashFunction(kerning.first, kerning.second, multiplier, arraySize);

        let currentCollisions = 0;

        while (sparseArray[index]) {
            index = (index + 1) % arraySize;
            currentCollisions = currentCollisions + 1;
        }

        maxCollisions = Math.max(maxCollisions, currentCollisions);

        sparseArray[index] = kerning;
    }

    for (let i = 0; i < arraySize; ++i) {
        if (!sparseArray[i]) {
            sparseArray[i] = {amount: 0, first: 0, second: 0};
        }
    }

    return {sparseArray, maxCollisions};
}

function searchForBestKerning(kerningList) {
    let result;
    let multiplier;

    let arraySize = 1;
    let mask = 2;

    while (arraySize < kerningList.length) {
        arraySize *= 2;
        mask <<= 1;
    }

    arraySize *= 2;

    for (let i = 1; i < 0x10000; ++i) {
        const check = checkForCollisions(kerningList, i, arraySize);

        if (!result || check.maxCollisions < result.maxCollisions) {
            result = check
            multiplier = i;
        }
    }

    console.log(`maxCollisions = ${result.maxCollisions}`);

    mask -= 1;

    return {result: result.sparseArray, multiplier: multiplier, mask: mask, maxCollisions: result.maxCollisions};
}

function buildKerning(kerningList) {
    return `struct FontKerning g${name}Kerning[] = {
${kerningList.map(kerning => `    {.amount = ${kerning.amount}, .first = ${kerning.first}, .second = ${kerning.second}},`).join('\n')}
};
`
}

function buildFont(multiplier, mask, symbolCount, maxCollisions) {
    return `struct Font g${name}Font = {
    .kerning = &g${name}Kerning[0],
    .symbols = &g${name}Symbols[0],
    .base = ${input.config.base},
    .charHeight = ${input.config.charHeight},
    .symbolCount = ${symbolCount},
    .kerningMultiplier = ${multiplier},
    .kerningMask = 0x${mask.toString(16)},
    .maxCollisions = ${maxCollisions},
};
`
}

function sparseSymbols(symbols) {
    const result = [];

    for (let i = 0; i < symbols.length; ++i) {
        result[symbols[i].id] = symbols[i];
    }

    for (let i = 0; i < result.length; ++i) {
        if (!result[i]) {
            result[i] = {
                x: 0, y: 0,
                width: 0, height: 0,
                xoffset: 0, yoffset: 0,
                xadvance: 0,
            };
        }
    }

    return result;
}

function buildSymbol(symbol) {
    return `    {
        .x = ${symbol.x}, .y = ${symbol.y},
        .width = ${symbol.width}, .height = ${symbol.height},
        .xoffset = ${symbol.xoffset}, .yoffset = ${symbol.yoffset},
        .xadvance = ${symbol.xadvance},
    },`
}

const kerningResult = searchForBestKerning(input.kerning);

const symbols = sparseSymbols(input.symbols);

fs.writeFileSync(process.argv[4], `

#include "font.h"

${buildKerning(kerningResult.result)}

struct FontSymbol g${name}Symbols[] = {
${symbols.map(buildSymbol).join('\n')}
};

${buildFont(kerningResult.multiplier, kerningResult.mask, symbols.length, kerningResult.maxCollisions)}
`);