const fs = require('fs');
const path = require('path');

const InvalidTokenCharacter = /[^A-Za-z0-9_]/gim;

function getSegmentName(objectLocation) {
    const levelName = path.basename(objectLocation);
    const noExtension = levelName.slice(0, levelName.length - path.extname(levelName).length);

    return noExtension.replace(InvalidTokenCharacter, '_');
}

function generateLD(segmentLocation, objectLocation) {
    const segmentName = getSegmentName(objectLocation);

    return `
    BEGIN_SEG(${segmentName}, ${segmentLocation})
    {
       ${objectLocation}(.data);
       ${objectLocation}(.bss);
    }
    END_SEG(${segmentName})
    `;
}

function generateData(segmentLocation, objectLocations) {
    return objectLocations.map(objectLocation => generateLD(segmentLocation, objectLocation)).join('\n');
}

const output = process.argv[2];
const segmentLocation = process.argv[3];

fs.writeFileSync(output, generateData(segmentLocation, process.argv.slice(4)));