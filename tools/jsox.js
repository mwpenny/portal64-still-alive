const fs = require('fs');
const child_process = require('child_process');

const fileContents = fs.readFileSync(process.argv[2]);
const fileJSON = JSON.parse(fileContents);

fileJSON.forEach((command) => {
    const commandText = `sox ${process.argv[3]} ${command.flags || ''} ${process.argv[4]} ${command.filters || ''}`;

    process.stdout.write(commandText);
    process.stdout.write('\n');
    const script = child_process.exec(commandText);

    script.stdout.on('data', function(data){
        process.stdout.write(data.toString());
    });
    // what to do with data coming from the standard error
    script.stderr.on('data', function(data){
        process.stderr.write(data.toString());
    });
});