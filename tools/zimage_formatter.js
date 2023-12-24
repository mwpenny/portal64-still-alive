const fs = require('fs');

function pad(idx, len) {
    let result = String(idx);

    while (result.length < len) {
        result = '0' + result;
    }

    return result;
}

function fixForImage(imageIdx) {
    const filename = process.argv[2] + String(imageIdx) + '.bmp';

    if (!fs.existsSync(filename)) {
        return;
    }

    const data = fs.readFileSync(filename);
    const imageInput = data.subarray(14 + 12);
    
    let idx = 0;
    
    const grayscale = [];
    
    const SCREEN_WD = 320;
    const SCREEN_HT = 240;
    
    for (let x = 0; x < SCREEN_WD; ++x) {
        for (let y = 0; y < SCREEN_HT; ++y) {
            grayscale.push(((imageInput[idx + 2] & 0xF8) << 7) | ((imageInput[idx + 1] & 0xF8) << 2) | ((imageInput[idx + 0] & 0xF8) >> 3));
    
            idx += 3;
        }
    }
    
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
    
        const value = Math.floor(grayscale[idx] >> 7);
    
        buffer[outIdx + 2] = value;
        buffer[outIdx + 1] = value;
        buffer[outIdx + 0] = value;
    }

    fs.writeFileSync(process.argv[3] + pad(String(imageIdx), 3) + '.bmp', buffer);
}


for (let i = 0; i <= 300; ++i) {
    fixForImage(i);
}