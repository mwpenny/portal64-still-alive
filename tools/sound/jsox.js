const child_process = require('child_process');
const fs = require('fs');
const path = require('path');

if (process.argv.length !== 7) {
    console.log('Converts sound files using sox and a JSON file containing arguments.\n');
    console.log(`Usage: ${process.argv[0]} ${process.argv[1]} SOX_PATH JSOX_FILE INPUT_FILE OUTPUT_FILE SAMPLE_RATE`);
    process.exit(1);
}

const [soxPath, argsFile, inputFile, outputFile, sampleRate] = process.argv.slice(2);

const fileContents = fs.readFileSync(argsFile);
const fileJSON = JSON.parse(fileContents);

for (const command of fileJSON) {

    if (command.flags !== null) {
        command.flags = `${command.flags} -r ${sampleRate}`
    }

    const commandText = `"${soxPath}" -V1 "${inputFile}" ${command.flags || ''} "${outputFile}" ${command.filters || ''}`;

    const outputParentDir = path.dirname(outputFile);
    if (!fs.existsSync(outputParentDir)) {
        fs.mkdirSync(outputParentDir, { recursive: true });
    }

    const script = child_process.exec(commandText);

    script.stdout.on('data', function(data) {
        process.stdout.write(data.toString());
    });
    script.stderr.on('data', function(data) {
        process.stderr.write(data.toString());
    });
    script.on('exit', function(code) {
        process.exit(code);
    });
}
