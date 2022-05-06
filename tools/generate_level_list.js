const fs = require('fs');
const path = require('path');

function generateInclude(outputLocation, headerLocation) {
    return `#include "${path.relative(path.dirname(outputLocation), headerLocation)}"`
}

const InvalidTokenCharacter = /[^A-Za-z0-9_]/gim;

function generateLevelName(outputLocation, headerLocation) {
    const relative = path.relative(path.dirname(outputLocation), headerLocation).slice(0, -2);
    return relative.replace(InvalidTokenCharacter, '_') + '_level';
}

function generateLevelList(outputLocation, headerLocations) {
    return `struct LevelDefinition* gLevelList[] = {
${headerLocations.map(headerLocation => `    &${generateLevelName(outputLocation, headerLocation)},`).join('\n')}
};
`;
}

function generateData(outputLocation, headerLocations) {
    return `#ifndef __BUILD_LEVEL_LIST_H__
#define __BUILD_LEVEL_LIST_H__

#include "levels/level_definition.h"

${headerLocations.map(headerLocation => generateInclude(outputLocation, headerLocation)).join('\n')}

#define LEVEL_COUNT ${headerLocations.length}

${generateLevelList(outputLocation, headerLocations)}

#endif
`;
}

const output = process.argv[2];

fs.writeFileSync(output, generateData(output, process.argv.slice(3)));