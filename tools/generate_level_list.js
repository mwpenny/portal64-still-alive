const fs = require('fs');
const path = require('path');

function generateInclude(outputLocation, headerLocation) {
    return `#include "${path.relative(path.dirname(outputLocation), headerLocation)}"`
}

function generateData(outputLocation, headerLocations) {
    return `#ifndef __BUILD_LEVEL_LIST_H__
#define __BUILD_LEVEL_LIST_H__

${headerLocations.map(headerLocation => generateInclude(outputLocation, headerLocation)).join('\n')}

#endif
`;
}

const output = process.argv[2];

fs.writeFileSync(output, generateData(output, process.argv.slice(3)));