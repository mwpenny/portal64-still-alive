const fs = require('fs');

const lines = fs.readFileSync(process.argv[2], 'utf-8').split('\n');

const symbolMapLines = fs.readFileSync(process.argv[3], 'utf-8').split('\n');

const lineParserRegexp = /(\d+)\/\d+ 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8}) ms (\d+\.\d+)/

const lineMappingRegexp = /addr 0x([a-f0-9]{8}) -> 0x([a-f0-9]{8})/

const symbolParserRegexp = /0x([a-f0-9]{16})\s+(\w+)/

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
const profileBatches = [];
let lastIndex = -1;

for (const line of lines) {
    const parsedLine = parseLine(line);

    if (parsedLine) {
        if (lastIndex != 0 && parsedLine.index == 0) {
            profileBatches.push({
                memoryMapping: new Map(memoryMapping),
                lines: [],
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
        memoryMapping.set(addrMatch[2], addrMatch[1]);
    }
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
    if (batch.memoryMapping.has(address)) {
        address = batch.memoryMapping.get(address);
    }

    return symbolAddressMapping.get(address) || `0x${address}`;
}

function formatCommandName(command, batch) {
    switch (command.command) {
        case 'db': 
        {
            const segment = parseInt(command.w0.substring(4), 16) / 4;
            return `gsSPSegment(0x${segment}, 0x${command.w1})`;
        }
        case 'de':
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