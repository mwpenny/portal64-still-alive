const fs = require('fs');
const path = require('path');

const InvalidTokenCharacter = /[^A-Za-z0-9_]/gim;

function getSegmentName(objectLocation) {
    const levelName = path.basename(objectLocation);
    const noExtension = levelName.slice(0, levelName.length - path.extname(levelName).length);

    return noExtension.replace(InvalidTokenCharacter, '_');
}

function generateLD(objectLocation) {
    const segmentName = getSegmentName(objectLocation);

    return `
    BEGIN_SEG(${segmentName}, 0x02000000)
    {
       ${objectLocation}(.data);
       ${objectLocation}(.bss);
    }
    END_SEG(${segmentName})
    `;
}

function generateData(objectLocations) {
    return objectLocations.map(objectLocation => generateLD(objectLocation)).join('\n');
}

const output = process.argv[2];

fs.writeFileSync(output, generateData(process.argv.slice(3)));