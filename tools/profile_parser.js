const fs = require('fs');

const lines = fs.readFileSync(process.argv[2], 'utf-8').split('\n');

const symbolMapLines = fs.readFileSync(process.argv[3], 'utf-8').split('\n');

const lineParserRegexp = /(\d+)\/\d+ 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8}) ms (\d+\.\d+)/

const lineMappingRegexp = /addr 0x([a-f0-9]{8}) -> (\w+)/

const dlRegexp = /dl d (\d+) 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8})/

const symbolParserRegexp = /0x([a-f0-9]{16})\s+(\w+)/

const G_DL = 'de';

function parseLine(line) {
    const match = lineParserRegexp.exec(line);

    if (!match) {
        return null;
    }

    return {
        index: parseInt(match[1]),
        command: match[2],
        w0: match[3],
        w1: match[4],
        startTime: parseFloat(match[5]),
    }
}

function parseSymbolLine(line) {
    const match = symbolParserRegexp.exec(line);

    if (!match) {
        return null;
    }

    return {
        address: match[1].substring(8),
        name: match[2],
    };
}

const memoryMapping = new Map();
const displayListStack = [];
let currentDisplayList = [];
const profileBatches = [];

let lastIndex = -1;

function checkDisplayList(line) {
    const dlMatch = dlRegexp.exec(line);

    if (!dlMatch) {
        return;
    }

    const depth = +dlMatch[1];

    while (displayListStack.length === 0 && depth === 0) {
        currentDisplayList = [];
        displayListStack.push(currentDisplayList);
    }

    const current = displayListStack[depth];

    if (!current) {
        throw new Error('malformed display list');
    }

    const command = {
        command: dlMatch[2],
        w0: dlMatch[3],
        w1: dlMatch[4],
    };

    if (command.command == G_DL) {
        const nextList = [];
        command.child = nextList;
        displayListStack.push(nextList);
    }

    if (command.command == 'df') {
        displayListStack.pop();
    }

    current.push(command);
}


const symbolAddressMapping = new Map();

for (const symbolLine of symbolMapLines) {
    const parsedLine = parseSymbolLine(symbolLine);

    if (!parsedLine) {
        continue;
    }

    if (symbolAddressMapping.has(parsedLine.address)) {
        symbolAddressMapping.set(parsedLine.address, symbolAddressMapping.get(parsedLine.address) + ',' + parsedLine.name);
    } else {
        symbolAddressMapping.set(parsedLine.address, parsedLine.name);
    }
}

function findChildDisplayLists(dl, memoryMapping, result) {
    for (const command of dl) {
        if (command.command == G_DL) {
            const address = command.w1;
            const dlName = memoryMapping.get(address) || symbolAddressMapping.get(address);
            if (dlName) {
                result.push(dlName);
            } else {
                findChildDisplayLists(command.child, memoryMapping, result);   
            }
        }
    }
}

for (const line of lines) {
    const parsedLine = parseLine(line);

    if (parsedLine) {
        if (lastIndex != 0 && parsedLine.index == 0) {
            const memoryMappingCopy = new Map(memoryMapping);

            for (const command of currentDisplayList) {
                if (command.command == G_DL) {
                    const children = [];
                    findChildDisplayLists(command.child, memoryMappingCopy, children);

                    if (children.length) {
                        memoryMappingCopy.set(command.w1, children.join('+'));
                    }
                }
            }

            profileBatches.push({
                memoryMapping: memoryMappingCopy,
                lines: [],
                currentDisplayList,
            })
        }

        const latestBatch = profileBatches[profileBatches.length - 1];
        latestBatch.lines.push(parsedLine);

        lastIndex = parsedLine.index;
        continue
    }

    if (line.trim() == 'addr clearall') {
        memoryMapping.clear();
    }

    const addrMatch = lineMappingRegexp.exec(line);

    if (addrMatch) {
        memoryMapping.set(addrMatch[1], addrMatch[2]);
    }

    checkDisplayList(line);
}

function calculateAverage(batch) {
    const combinedCommands = [];
    
    for (const parsedLine of batch.lines) {
        const existing = combinedCommands[parsedLine.index];
    
        if (existing) {
            existing.startTime += parsedLine.startTime;
            existing.total += 1;
        } else {
            combinedCommands[parsedLine.index] = {
                ...parsedLine,
                total: 1,
            };
        }
    }
    
    for (let i = 0; i < combinedCommands.length; ++i) {
        const current = combinedCommands[i];
    
        if (current) {
            current.startTime /= current.total;
        }
    }
    
    for (let i = 0; i + 1 < combinedCommands.length; ++i) {
        const current = combinedCommands[i];
        const next = combinedCommands[i + 1];
    
        current.elapsedTime = next.startTime - current.startTime;
    }
    
    // the last command is always a pipe sync we dont care about
    combinedCommands.pop();
    
    combinedCommands.sort((a, b) => b.elapsedTime - a.elapsedTime);

    batch.combinedCommands = combinedCommands;
}

profileBatches.forEach(calculateAverage);

function formatAddress(address, batch) {
    return batch.memoryMapping.get(address) || symbolAddressMapping.get(address) || `0x${address}`;
}

function formatCommandName(command, batch) {
    switch (command.command) {
        case 'db': 
        {
            const segment = parseInt(command.w0.substring(4), 16) / 4;
            return `gsSPSegment(0x${segment}, 0x${command.w1})`;
        }
        case G_DL:
            return `gsSPDisplayList(${formatAddress(command.w1, batch)})`;
        case 'f6':
            return `gsDPFillRectangle`;
        case 'd8':
            return `gsSPPopMatrix`;
        case 'da':
            return `gsSPMatrix`;
        default:
            return `unknown 0x${command.command} 0x${command.w0}${command.w1}`;
    }
}

function formatCommand(command, batch) {
    return `${command.elapsedTime} ${formatCommandName(command, batch)}`;
}

for (const batch of profileBatches) {    
    console.log('start of batch');
    for (const command of batch.combinedCommands) {
        console.log(formatCommand(command, batch));
    }
    console.log('end of batch');
}