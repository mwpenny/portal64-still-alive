const fs = require('fs');
const path = require('path');

function generateInclude(outputLocation, headerLocation) {
    return `#include "${path.relative(path.dirname(outputLocation), headerLocation)}"`
}

const InvalidTokenCharacter = /[^A-Za-z0-9_]/gim;

function getSegmentName(headerLocation) {
    const levelName = path.basename(headerLocation);
    const noExtension = levelName.slice(0, levelName.length - path.extname(levelName).length);

    return noExtension.replace(InvalidTokenCharacter, '_');
}

function generateLevelName(outputLocation, headerLocation) {
    const relative = path.relative(path.dirname(outputLocation), headerLocation).slice(0, -2);
    return relative.replace(InvalidTokenCharacter, '_') + '_level';
}

function generateMetadata(outputLocation, headerLocation) {
    const segmentName = getSegmentName(headerLocation);
    return `    {
        &${generateLevelName(outputLocation, headerLocation)},
        _${segmentName}_geoSegmentRomStart,
        _${segmentName}_geoSegmentRomEnd,
        _${segmentName}_geoSegmentStart,
    },`;
}

function generateExterns(headerLocation) {
    const segmentName = getSegmentName(headerLocation);
    return `
extern char _${segmentName}_geoSegmentRomStart[];
extern char _${segmentName}_geoSegmentRomEnd[];
extern char _${segmentName}_geoSegmentStart[];
`;

}

function generateLevelList(outputLocation, headerLocations) {
    return `struct LevelMetadata gLevelList[] = {
${headerLocations.map(headerLocation => generateMetadata(outputLocation, headerLocation)).join('\n')}
};
`;
}

function generateData(outputLocation, headerLocations) {
    return `#ifndef __BUILD_LEVEL_LIST_H__
#define __BUILD_LEVEL_LIST_H__

#include "levels/level_metadata.h"

${headerLocations.map(headerLocation => generateInclude(outputLocation, headerLocation)).join('\n')}

#define LEVEL_COUNT ${headerLocations.length}

${headerLocations.map(generateExterns).join('\n')}

${generateLevelList(outputLocation, headerLocations)}

#endif
`;
}

const output = process.argv[2];

fs.writeFileSync(output, generateData(output, process.argv.slice(3)));