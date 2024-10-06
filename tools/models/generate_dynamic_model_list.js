const fs = require('fs');
const path = require('path');
const util = require('./model_list_utils');

function generateModelListEntry(outputPath, modelHeader) {
    const modelName = util.generateModelName(modelHeader);
    return `    {
        _${modelName}_geoSegmentRomStart,
        _${modelName}_geoSegmentRomEnd,
        _${modelName}_geoSegmentStart,
        ${util.generateRelativeModelName(outputPath, modelHeader, '_model_gfx')},
        "${modelName}",
    },`;
}

const [outputHeaderFile, ...modelHeaders] = process.argv.slice(2);
const { dir: outputDir, name: outputName } = path.parse(outputHeaderFile);
const outputSourceFile = `${outputDir}/${outputName}.c`

const config = {
    modelHeaders,
    modelGroup: "dynamic_model",
    modelType: "DynamicAssetModel",
    listEntryGenerator: generateModelListEntry
};

fs.writeFileSync(
    outputHeaderFile,
    util.generateHeader(outputHeaderFile, config)
);
fs.writeFileSync(
    outputSourceFile,
    util.generateData(outputSourceFile, config)
);
