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
        console.log('malformed display list');
        return;
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

const imageCost = [];

const SCREEN_WD = 320;
const SCREEN_HT = 240;

for (let i = 0; i < SCREEN_WD * SCREEN_HT; ++i) {
    imageCost.push(0);
}

function calculateAverage(batch) {
    let combinedCommands = [];
    
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

        if (current && fs.existsSync(`log_images/step_${i}.bmp`)) {
            const data = fs.readFileSync(`log_images/step_${i}.bmp`);
            current.imageData = data.subarray(14 + 12);
            console.log(`log_images/step_${i}.bmp`);
        }
    }
    
    for (let i = 0; i + 1 < combinedCommands.length; ++i) {
        const current = combinedCommands[i];
        const next = combinedCommands[i + 1];

        if (!current || !next) {
            continue;
        }
    
        current.elapsedTime = next.startTime - current.startTime;

        let pixelDiffCount = 0;

        if (current.imageData && next.imageData) {
            for (let idx = 0; idx < SCREEN_HT * SCREEN_WD * 3; idx += 3) {
                if (current.imageData[idx + 0] != next.imageData[idx + 0] ||
                    current.imageData[idx + 1] != next.imageData[idx + 1] ||
                    current.imageData[idx + 2] != next.imageData[idx + 2]) {
                    ++pixelDiffCount;
                }
            }

            if (pixelDiffCount == 0) {
                continue;
            }

            const pixelCost = current.elapsedTime / pixelDiffCount;

            for (let y = 0; y < SCREEN_HT; ++y) {
                for (let x = 0; x < SCREEN_WD; ++x) {
                    const idx = (x + y * SCREEN_WD) * 3;

                    if (current.imageData[idx + 0] != next.imageData[idx + 0] ||
                        current.imageData[idx + 1] != next.imageData[idx + 1] ||
                        current.imageData[idx + 2] != next.imageData[idx + 2]) {
                            imageCost[x + y * SCREEN_WD] += pixelCost;
                    }
                }
            }
        }
    }
    
    // the last command is always a pipe sync we dont care about
    combinedCommands.pop();
    
    combinedCommands.sort((a, b) => b.elapsedTime - a.elapsedTime);

    combinedCommands = combinedCommands.filter(Boolean);

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
    return `${command.elapsedTime} ${command.index} ${formatCommandName(command, batch)}`;
}

for (const batch of profileBatches) {    
    console.log('start of batch');
    for (const command of batch.combinedCommands) {
        console.log(formatCommand(command, batch));
    }
    console.log('end of batch');
}

const maxCost = imageCost.reduce((a, b) => Math.max(a, b), 0);

const headerSize = 14;
const dataSize = SCREEN_WD * SCREEN_HT * 3;

const dibHeaderSize = 12;

const buffer = Buffer.alloc(headerSize + dibHeaderSize + dataSize);

buffer[0] = 0x42;
buffer[1] = 0x4D;

buffer.writeUInt32LE(buffer.length, 2);
buffer.writeUInt16LE(0, 6);
buffer.writeUInt16LE(0, 8);
buffer.writeUInt32LE(headerSize + dibHeaderSize, 10);

buffer.writeUInt32LE(12, 14);
buffer.writeUInt16LE(SCREEN_WD, 18);
buffer.writeUInt16LE(SCREEN_HT, 20);
buffer.writeUInt16LE(1, 22);
buffer.writeUInt16LE(24, 24);

for (let idx = 0; idx < SCREEN_WD * SCREEN_HT; ++idx) {
    const outIdx = headerSize + dibHeaderSize + idx * 3;

    const value = Math.floor(255 * imageCost[idx] / maxCost + 0.5);

    buffer[outIdx + 2] = value;
    buffer[outIdx + 1] = value;
    buffer[outIdx + 0] = value;
}

fs.writeFileSync('log_images/pixel_cost.bmp', buffer);