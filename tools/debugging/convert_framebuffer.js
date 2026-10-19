const fs = require("fs");
const path = require("path");

const SCREEN_WD = 320;
const SCREEN_HT = 240;

const BMP_HEADER_SIZE = 14;
const BMP_DIB_HEADER_SIZE = 12;
const BMP_BYTES_PER_PIXEL = 3;

function convertColor(hi, lo, isDepth) {
    if (isDepth) {
        const depth = ((hi << 8) | lo) / (Math.pow(2, 16) - 1);
        const value = 255 - Math.trunc(depth * 255);
        return [value, value, value];
    } else {
        const r = Math.trunc(255 * ((hi >> 3) & 0x1f) / 31);
        const g = Math.trunc(255 * (((hi << 2) & 0x1c) | ((lo >> 6) & 0x3)) / 31);
        const b = Math.trunc(255 * ((lo >> 1) & 0x1f) / 31);
        return [b, g, r];
    }
}

function writeBitmap(inputFile, outputFile, isDepth) {
    const dataOffset = BMP_HEADER_SIZE + BMP_DIB_HEADER_SIZE;
    const dataSize = SCREEN_WD * SCREEN_HT * BMP_BYTES_PER_PIXEL;
    const buffer = Buffer.alloc(dataOffset + dataSize);

    // Header
    buffer.write("BM", 0);
    buffer.writeUInt32LE(buffer.length, 2);
    buffer.writeUInt32LE(0, 6);
    buffer.writeUInt32LE(dataOffset, 10);

    // DIB header
    buffer.writeUInt32LE(BMP_DIB_HEADER_SIZE, 14);
    buffer.writeUInt16LE(SCREEN_WD, 18);
    buffer.writeUInt16LE(SCREEN_HT, 20);
    buffer.writeUInt16LE(1, 22);
    buffer.writeUInt16LE(BMP_BYTES_PER_PIXEL * 8, 24);

    // Data
    const pixelData = fs.readFileSync(inputFile);

    for (let row = 0; row < SCREEN_HT; ++row) {
        for (let col = 0; col < SCREEN_WD; ++col) {
            // Bitmaps store lines in reverse order
            const inRow = SCREEN_HT - row - 1;
            const inIdx = ((SCREEN_WD * inRow) + col) * 2;

            const [hi, lo] = pixelData.slice(inIdx, inIdx + 2);
            const px = convertColor(hi, lo, isDepth);

            const outIdx = dataOffset + ((SCREEN_WD * row) + col) * BMP_BYTES_PER_PIXEL;
            for (let i = 0; i < BMP_BYTES_PER_PIXEL; ++i) {
                buffer.writeUInt8(px[i], outIdx + i);
            }
        }
    }

    fs.writeFileSync(outputFile, buffer);
}


if (process.argv.length < 4) {
    console.log("Converts framebuffer data into bitmap images.\n");
    console.log(`Usage: ${path.basename(process.argv[1])} INPUT_FILE OUTPUT_FILE [--is-depth]`);
    process.exit(1);
}

let [inputFile, outputFile, isDepth] = process.argv.slice(2);
isDepth = (["--is-depth", "-d"].includes(isDepth));
writeBitmap(inputFile, outputFile, isDepth);
