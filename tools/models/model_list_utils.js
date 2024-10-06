const path = require('path');

const INVALID_TOKEN_CHARACTER = /[^A-Za-z0-9_]/gim;

// Common

function sanitize(s) {
    return s.replace(INVALID_TOKEN_CHARACTER, '_');
}

function wrapWithIncludeGuard(outputPath, content) {
    const outputName = path.basename(outputPath);
    const includeGuardName = `__${sanitize(outputName).toUpperCase()}__`;
    return `#ifndef ${includeGuardName}
#define ${includeGuardName}

${content}

#endif
`;
}

function generateRelativeModelName(outputPath, modelHeader, suffix='') {
    const outputDirName = path.dirname(outputPath);
    const { dir: headerDirName, name: headerName } = path.parse(modelHeader);

    const relative = path.join(
        path.relative(outputDirName, headerDirName),
        headerName
    );

    return sanitize(relative + suffix);
}

function generateModelName(modelHeader) {
    return generateRelativeModelName(modelHeader, modelHeader);
}

function generateListDefinition(modelGroup, modelType) {
    const groupPascalCase = modelGroup
        .split('_')
        .map(s => s && s.replace(/^(\w)/g, m => m.toUpperCase()))
        .join('');

    const listName = `g${groupPascalCase}s`;
    return `struct ${modelType} ${listName}[]`;
}

// Header generation

function generateCount(modelGroup, modelHeaders) {
    return `#define ${modelGroup.toUpperCase()}_COUNT ${modelHeaders.length}`;
}

function generateModelIndices(outputPath, modelHeaders, modelGroup) {
    return modelHeaders.map((modelHeader, index) => {
        const modelName = generateRelativeModelName(outputPath, modelHeader, `_${modelGroup}`);
        return `#define ${modelName.toUpperCase()} ${index}`;
    }).join('\n');
}

function generateListExtern(modelGroup, modelType) {
    return `extern ${generateListDefinition(modelGroup, modelType)};`;
}

function generateHeader(outputPath, config) {
    const { modelHeaders, modelGroup, modelType } = config;

    return wrapWithIncludeGuard(outputPath,
`#include "util/dynamic_asset_model.h"

${generateCount(modelGroup, modelHeaders)}

${generateModelIndices(outputPath, modelHeaders, modelGroup)}

${generateListExtern(modelGroup, modelType)}`
    );
}

// Data generation

function generateIncludes(outputPath, modelHeaders) {
    return modelHeaders.map(modelHeader => {
        const relativePath = path.relative(path.dirname(outputPath), modelHeader);
        return `#include "${relativePath}"`;
    }).join('\n');
}

function generateExterns(modelHeaders) {
    return modelHeaders.map(modelHeader => {
        const modelName = generateModelName(modelHeader);
        return `extern char _${modelName}_geoSegmentRomStart[];
extern char _${modelName}_geoSegmentRomEnd[];
extern char _${modelName}_geoSegmentStart[];`;
    }).join('\n\n');
}

function generateModelList(outputPath, config) {
    const { modelHeaders, modelGroup, modelType, listEntryGenerator } = config;

    return `${generateListDefinition(modelGroup, modelType)} = {
${modelHeaders.map(modelHeader => listEntryGenerator(outputPath, modelHeader)).join('\n')}
};`
}

function generateData(outputPath, config) {
    const { name } = path.parse(outputPath);
    const { modelHeaders } = config;

    return `#include "${name}.h"

${generateIncludes(outputPath, modelHeaders)}

${generateExterns(modelHeaders)}

${generateModelList(outputPath, config)}
`;
}

module.exports = {
    wrapWithIncludeGuard,
    generateRelativeModelName,
    generateModelName,

    generateCount,
    generateHeader,

    generateIncludes,
    generateExterns,
    generateModelList,
    generateData
};
