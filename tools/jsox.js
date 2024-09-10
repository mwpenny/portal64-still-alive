const child_process = require('child_process');
const fs = require('fs');
const path = require('path');

if (process.argv.length !== 5) {
    console.log('Converts sound files using sox and a JSON file containing arguments.\n');
    console.log(`Usage: ${process.argv[0]} ${process.argv[1]} JSOX_FILE INPUT_FILE OUTPUT_FILE`);
    process.exit(1);
}

const [argsFile, inputFile, outputFile] = process.argv.slice(2);

const fileContents = fs.readFileSync(argsFile);
const fileJSON = JSON.parse(fileContents);

fileJSON.forEach((command) => {
    const commandText = `sox ${inputFile} ${command.flags || ''} ${outputFile} ${command.filters || ''}`;

    const outputParentDir = path.dirname(outputFile);
    if (!fs.existsSync(outputParentDir)) {
        fs.mkdirSync(outputParentDir, { recursive: true });
    }

    //process.stdout.write(commandText);
    //process.stdout.write('\n');
    const script = child_process.exec(commandText);

    script.stdout.on('data', function(data){
        process.stdout.write(data.toString());
    });
    // what to do with data coming from the standard error
    script.stderr.on('data', function(data){
        process.stderr.write(data.toString());
    });
});