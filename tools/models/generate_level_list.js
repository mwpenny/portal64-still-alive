const fs = require('fs');
const util = require('./model_list_utils');

function generateLevelListEntry(outputPath, levelHeader) {
    const levelName = util.generateModelName(levelHeader);
    return `    {
        &${util.generateRelativeModelName(outputPath, levelHeader, '_level')},
        _${levelName}_geoSegmentRomStart,
        _${levelName}_geoSegmentRomEnd,
        _${levelName}_geoSegmentStart,
    },`;
}

function generateHeader(outputPath, config) {
    const { modelHeaders, modelGroup } = config;

    return util.wrapWithIncludeGuard(outputPath,
`#include "levels/level_metadata.h"

${util.generateIncludes(outputPath, modelHeaders)}

${util.generateCount(modelGroup, modelHeaders)}

${util.generateExterns(modelHeaders)}

${util.generateModelList(outputPath, config)}`
    );
}

const [outputHeaderFile, ...modelHeaders] = process.argv.slice(2);

const config = {
    modelHeaders,
    modelGroup: "level",
    modelType: "LevelMetadata",
    listEntryGenerator: generateLevelListEntry
};

fs.writeFileSync(
    outputHeaderFile,
    generateHeader(outputHeaderFile, config)
);
