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

function generateArmatureName(outputLocation, headerLocation) {
    const relative = path.relative(path.dirname(outputLocation), headerLocation).slice(0, -2);
    return relative.replace(InvalidTokenCharacter, '_') + '_armature';
}

function generateClipsName(outputLocation, headerLocation) {
    const relative = path.relative(path.dirname(outputLocation), headerLocation).slice(0, -2);
    return relative.replace(InvalidTokenCharacter, '_') + '_clips';
}

function generateClipCountName(outputLocation, headerLocation) {
    const relative = path.relative(path.dirname(outputLocation), headerLocation).slice(0, -2);
    return relative.replace(InvalidTokenCharacter, '_').toUpperCase() + '_CLIP_COUNT';
}

function generateDynamicModelName(outputLocation, headerLocation, index) {
    const relative = path.relative(path.dirname(outputLocation), headerLocation).slice(0, -2);
    return relative.replace(InvalidTokenCharacter, '_') + '_DYNAMIC_ANIMATED_MODEL';
}

function generateMetadata(outputLocation, headerLocation) {
    const segmentName = getSegmentName(headerLocation);
    return `    {
        _${segmentName}_geoSegmentRomStart,
        _${segmentName}_geoSegmentRomEnd,
        _${segmentName}_geoSegmentStart,
        &${generateArmatureName(outputLocation, headerLocation)},
        ${generateClipsName(outputLocation, headerLocation)},
        ${generateClipCountName(outputLocation, headerLocation)},
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

function generateModelList(outputLocation, headerLocations) {
    return `struct DynamicAnimatedAssetModel gDynamicAnimatedModels[] = {
${headerLocations.map(headerLocation => generateMetadata(outputLocation, headerLocation)).join('\n')}
};`
}

function generateMetadataDeclaration(outputLocation, headerLocation, index) {
    const segmentName = getSegmentName(headerLocation);
    return `#define ${generateDynamicModelName(outputLocation, headerLocation).toUpperCase()} ${index}`;
}

function generateModelHeaderList(outputLocation, headerLocations) {
    return headerLocations.map((headerLocation, index) => generateMetadataDeclaration(outputLocation, headerLocation, index)).join('\n');
}

function generateData(outputLocation, headerLocations) {
    return `#ifndef __DYNAMIC_ANIMATED_MODEL_LIST_DATA_H__
#define __DYNAMIC_ANIMATED_MODEL_LIST_DATA_H__

#include "util/dynamic_asset_loader.h"

${headerLocations.map(headerLocation => generateInclude(outputLocation, headerLocation)).join('\n')}

${headerLocations.map(generateExterns).join('\n')}

${generateModelList(outputLocation, headerLocations)}

#endif
`;
}

function generateHeader(outputLocation, headerLocations) {
    return `#ifndef __DYNAMIC_ANIMATED_MODEL_LIST_DEFINITION_H__
#define __DYNAMIC_ANIMATED_MODEL_LIST_DEFINITION_H__

#define DYNAMIC_ANIMATED_MODEL_COUNT ${headerLocations.length}

${generateModelHeaderList(outputLocation, headerLocations)}

#endif
`;
}

fs.writeFileSync(process.argv[2], generateHeader(process.argv[2], process.argv.slice(4)));
fs.writeFileSync(process.argv[3], generateData(process.argv[3], process.argv.slice(4)));