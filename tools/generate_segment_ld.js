const fs = require('fs');
const path = require('path');
const util = require('util');

const INVALID_TOKEN_CHARACTER = /[^A-Za-z0-9_]/gim;

function sanitize(s) {
    return s.replace(INVALID_TOKEN_CHARACTER, '_');
}

function generateSegmentNameFromObject(objectPath) {
    const { name } = path.parse(objectPath);
    return sanitize(name.split('.')[0]);
}

function generateSegmentContent(objectPath) {
    return `
        ${objectPath}(.data)
        ${objectPath}(.bss)
        ${objectPath}(.rodata*)
    `.trim();
}

function generateAlign(alignment) {
    return `__romPos = ALIGN(__romPos, ${alignment});\n`;
}

function generateSegment(segmentName, loadAddress, alignment, objectPaths) {
    const align = alignment ? `    ${generateAlign(alignment)}` : '';
    return `${align}    BEGIN_SEG(${segmentName}, ${loadAddress})
    {
        ${objectPaths.map(generateSegmentContent).join('\n        ')}
    }
    END_SEG(${segmentName})
`;
}

function generateMultiSegments(loadAddress, alignment, objectPaths) {
    return objectPaths.map(objectPath => {
        const segmentName = generateSegmentNameFromObject(objectPath);
        return generateSegment(segmentName, loadAddress, alignment, [objectPath]);
    }).join('\n');
}

const { values, positionals } = util.parseArgs({
    options: {
        'single-segment-name': {
            type: 'string'
        },
        'alignment': {
            type: 'string'
        }
    },
    allowPositionals: true
});

const [outputLinkerScript, loadAddress, ...objectPaths] = positionals;
const singleSegmentName = values['single-segment-name'];
const alignment = values['alignment'];

const outputParentDir = path.dirname(outputLinkerScript);
if (!fs.existsSync(outputParentDir)) {
    fs.mkdirSync(outputParentDir, { recursive: true });
}

const output = singleSegmentName ?
    generateSegment(singleSegmentName, loadAddress, alignment, objectPaths) :
    generateMultiSegments(loadAddress, alignment, objectPaths);
fs.writeFileSync(outputLinkerScript, output);
