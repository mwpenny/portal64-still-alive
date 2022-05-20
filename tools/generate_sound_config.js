const fs = require('fs');
const path = require('path');

for (const wav of fs.readdirSync(process.argv[2])) {
    fs.writeFileSync(path.join(process.argv[3], wav.slice(0, -4)) + '.sox', process.argv[4]);
}