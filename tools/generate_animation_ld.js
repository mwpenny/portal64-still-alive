const fs = require('fs');
const path = require('path');

function generateLD(objectLocation) {
    return `
       ${objectLocation}(.data);
       ${objectLocation}(.bss);
    `;
}

function generateData(objectLocations) {

    return `
    BEGIN_SEG(animation_segment, 0x0D000000)
    {
        ${objectLocations.map(objectLocation => generateLD(objectLocation)).join('\n')}
    }
    END_SEG(animation_segment)
    `;
}

const output = process.argv[2];

fs.writeFileSync(output, generateData(process.argv.slice(3)));