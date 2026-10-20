// This script parses RSP profiler output to generate a summary report and
// heatmap of frame performance hot spots.
//
// Usage:
// 1. Build the game with hardware debugging and the RSP profiler enabled.
//
// 2. While debugging the game, press d-pad down on controller 3 to log timing
//    information and save screenshots for the last frame's display list.
//
// 3. Run this script with the debug log file passed via --debug-log-file. It
//    will output a display list command cost report for each profiled frame.
//
//    The log file is also used to label dynamic model display lists. Optionally
//    provide portal.map from the build directory using --symbol-map-file to
//    label other display lists as well.
//
//    Specify the saved screenshots' directory with --screenshot-dir to generate
//    heatmaps highlighting pixel costs.
//
// Caveats:
// * Only commands from the top-level display list are included in cost reports.
//   The profiler does not recursively measure child performance.
//
// * Due to performance fluctuations and the way time is measured, some fast
//   commands can be shown with negative run times.
//
// * Dynamically loaded elements such as level geometry can share addresses with
//   each other. Such display lists are labelled using a comma-separated list of
//   all matching symbol names.
//
// * The game builds some display lists at runtime, meaning they have no symbol
//   names. They are labelled using a +-separated list of all named children
//   (recursively). The top-level address is used when no names are found.
//
// * Heatmap pixel values are computed by averaging each display list command's
//   cost across all pixels it affects, then summing the results.

const fs = require("fs");
const path = require("path");
const util = require("util");

const SCREEN_WD = 320;
const SCREEN_HT = 240;

class SymbolMap {
    static STATIC_SYMBOL_REGEX = /^0x([a-f0-9]+)\s+([a-zA-Z_]\w+)$/;
    static DYNAMIC_ASSET_RESET_REGEX = /^Reset dynamic assets$/
    static DYNAMIC_ASSET_LOAD_REGEX = /^Loaded dynamic asset at 0x([a-f0-9]{8}): (\w+)$/

    constructor(filePath) {
        this._staticSymbols = filePath
            ? SymbolMap._parseSymbolFile(filePath)
            : new Map();
        this._dynamicSymbols = new Map();
    }

    static _parseStaticSymbol(line) {
        const match = line.match(SymbolMap.STATIC_SYMBOL_REGEX);
        if (!match) {
            return null;
        }

        return {
            address: Number.parseInt(match[1], 16),
            name: match[2]
        };
    }

    static _parseSymbolFile(filePath) {
        const addressToSymbol = new Map();
        const lines = fs.readFileSync(filePath, "utf-8").split("\n");

        for (const line of lines) {
            const symbol = SymbolMap._parseStaticSymbol(line.trim());
            if (symbol) {
                // Dynamic elements can share the same address (multiple names)
                const names = [addressToSymbol.get(symbol.address), symbol.name];
                addressToSymbol.set(symbol.address, names.filter(Boolean).join(","))
            }
        }

        return addressToSymbol;
    }

    static _isDynamicAssetResetLine(line) {
        return SymbolMap.DYNAMIC_ASSET_RESET_REGEX.test(line);
    }

    static _parseDynamicAssetLoad(line) {
        const match = line.match(SymbolMap.DYNAMIC_ASSET_LOAD_REGEX);
        if (!match) {
            return null;
        }

        return {
            address: Number.parseInt(match[1], 16),
            name: match[2]
        }
    }

    tryUpdate(line) {
        if (SymbolMap._isDynamicAssetResetLine(line)) {
            this._dynamicSymbols.clear();
            return true;
        }

        const symbol = SymbolMap._parseDynamicAssetLoad(line);
        if (symbol) {
            this._dynamicSymbols.set(symbol.address, symbol.name);
            return true;
        }

        return false;
    }

    getSymbolName(address) {
        return this._staticSymbols.get(address) || this._dynamicSymbols.get(address);
    }
}

class DisplayList {
    static COMMAND_REGEX = /^dl 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8})$/;

    static G_FILLRECT = 0xf6;
    static G_POPMTX   = 0xd8;
    static G_MTX      = 0xda;
    static G_MOVEWORD = 0xdb;
    static G_DL       = 0xde;
    static G_ENDDL    = 0xdf;

    constructor(symbolMap) {
        this._symbolMap = symbolMap;
        this._stack = [null];  // Dummy element for root display list
        this._visited = new Set();
        this._childNames = new Map();
    }

    static _parseCommand(line) {
        const match = line.match(DisplayList.COMMAND_REGEX);
        if (!match) {
            return null;
        }

        return {
            command: Number.parseInt(match[1], 16),
            w0: Number.parseInt(match[2], 16),
            w1: Number.parseInt(match[3], 16),
            children: [],
            name: undefined
        };
    }

    _update(command) {
        switch (command.command) {
            case DisplayList.G_DL:
                const current = this._stack.at(-1);
                if (current) {
                    current.children.push(command);
                }
                this._stack.push(command);
                break;

            case DisplayList.G_ENDDL:
                if (this._stack.length === 0) {
                    throw new Error("Malformed display list");
                }

                const dl = this._stack.pop();
                if (!dl || this._visited.has(dl.w1)) {
                    break;
                }

                // Returning from display list. Try to name it using found info.
                const name = this._symbolMap.getSymbolName(dl.w1) ||
                    dl.children.map(c => c.name).filter(Boolean).join("+");

                if (name) {
                    dl.name = name;
                    this._childNames.set(dl.w1, dl.name);
                }

                this._visited.add(dl.w1);
                break;
        }
    }

    tryUpdate(line) {
        const command = DisplayList._parseCommand(line);
        if (command) {
            this._update(command);
            return true;
        }

        return false;
    }

    getCommandName(command) {
        const hexStr = (num, pad=0) => `${num.toString(16).padStart(pad, "0")}`;

        switch (command.command) {
            case DisplayList.G_FILLRECT:
                return `gsDPFillRectangle`;
            case DisplayList.G_POPMTX:
                return `gsSPPopMatrix`;
            case DisplayList.G_MTX:
                return `gsSPMatrix`;
            case DisplayList.G_MOVEWORD:
                const segmentNum = Math.trunc(command.w0 / 4) & 0xf;
                return `gsSPSegment(0x${hexStr(segmentNum)}, 0x${hexStr(command.w1, 8)})`;
            case DisplayList.G_DL:
                const address = this._childNames.get(command.w1) || `0x${hexStr(command.w1, 8)}`;
                return `gsSPDisplayList(${address})`;
            default:
                return `unknown 0x${hexStr(command.command)} 0x${hexStr(command.w0, 8)}${hexStr(command.w1, 8)}`;
        }
    }
}

class Heatmap {
    static BMP_HEADER_SIZE = 14;
    static BMP_DIB_HEADER_SIZE = 12;
    static BMP_BYTES_PER_PIXEL = 3;

    constructor() {
        this._pixelCosts = new Map();
    }

    update(pixelIndices, elementCost) {
        if (pixelIndices.length === 0 || elementCost <= 0) {
            return;
        }

        // Highlight elements with high per-pixel cost.
        //
        // Large expensive objects are more tolerable than small ones, as they
        // cover more of the screen. Small objects are more likely to appear
        // multiple times, or alongside other expensive objects.
        const averageCost = elementCost / pixelIndices.length;

        for (const i of pixelIndices) {
            const existing = this._pixelCosts.get(i) || 0;
            this._pixelCosts.set(i, existing + averageCost);
        }
    }

    isEmpty() {
        return this._pixelCosts.size === 0;
    }

    writeBitmap(filePath) {
        const maxCost = Math.max(...this._pixelCosts.values());

        const dataOffset = Heatmap.BMP_HEADER_SIZE + Heatmap.BMP_DIB_HEADER_SIZE;
        const dataSize = SCREEN_WD * SCREEN_HT * Heatmap.BMP_BYTES_PER_PIXEL;
        const buffer = Buffer.alloc(dataOffset + dataSize);

        // Header
        buffer.write("BM", 0);
        buffer.writeUInt32LE(buffer.length, 2);
        buffer.writeUInt32LE(0, 6);
        buffer.writeUInt32LE(dataOffset, 10);

        // DIB header
        buffer.writeUInt32LE(Heatmap.BMP_DIB_HEADER_SIZE, 14);
        buffer.writeUInt16LE(SCREEN_WD, 18);
        buffer.writeUInt16LE(SCREEN_HT, 20);
        buffer.writeUInt16LE(1, 22);
        buffer.writeUInt16LE(Heatmap.BMP_BYTES_PER_PIXEL * 8, 24);

        // Data
        for (let row = 0; row < SCREEN_HT; ++row) {
            for (let col = 0; col < SCREEN_WD; ++col) {
                // Bitmaps store lines in reverse order
                const inRow = SCREEN_HT - row - 1;
                const inIdx = (SCREEN_WD * inRow) + col;

                const cost = this._pixelCosts.get(inIdx) || 0;
                const value = Math.trunc((255 * cost / maxCost) + 0.5);

                const outIdx = dataOffset + ((SCREEN_WD * row) + col) * Heatmap.BMP_BYTES_PER_PIXEL;
                for (let i = 0; i < Heatmap.BMP_BYTES_PER_PIXEL; ++i) {
                    buffer.writeUInt8(value, outIdx + i);
                }
            }
        }

        fs.writeFileSync(filePath, buffer);
    }
}

class CommandInfo {
    static SAMPLE_REGEX = /^(\d+)\/\d+ 0x([a-f0-9]{2})([a-f0-9]{6})([a-f0-9]{8}) (\d+\.?\d+) ms$/;
    static SCREENSHOT_NAME_REGEX = /^.+'(.+\.bin)'.+$/;

    constructor(commandIndex, command, w0, w1, startTimeMs) {
        this.commandIndex = commandIndex;
        this.command = command;
        this.w0 = w0;
        this.w1 = w1;
        this.startTimeMs = startTimeMs;
        this.sampleCount = 1;

        this.runTimeMs = 0;
        this.screenshot = null;
        this.diffPixels = [];
    }

    static parse(line) {
        const match = line.match(CommandInfo.SAMPLE_REGEX);
        if (!match) {
            return null;
        }

        return new CommandInfo(
            Number(match[1]) - 1,
            Number.parseInt(match[2], 16),
            Number.parseInt(match[3], 16),
            Number.parseInt(match[4], 16),
            Number(match[5])
        );
    }

    tryUpdate(line, screenshotDir) {
        const match = line.match(CommandInfo.SCREENSHOT_NAME_REGEX);
        if (!match) {
            return false;
        }

        const screenshotPath = path.join(screenshotDir, match[1]);
        if (fs.existsSync(screenshotPath)) {
            // Load up front for performance
            // This will stay in memory longer than needed, but it is simpler
            this.screenshot = fs.readFileSync(screenshotPath);
        }

        return true;
    }

    accumulate(other) {
        this.startTimeMs += other.startTimeMs;
        this.sampleCount += other.sampleCount;
    }

    finish() {
        this.startTimeMs /= this.sampleCount;
    }

    computeDiff(next) {
        this.runTimeMs = next.startTimeMs - this.startTimeMs;

        if (this.screenshot && next.screenshot) {
            for (let pxIdx = 0; pxIdx < SCREEN_HT * SCREEN_WD; ++pxIdx) {
                const bufIdx = pxIdx * 2;

                if (this.screenshot[bufIdx + 0] !== next.screenshot[bufIdx + 0] ||
                    this.screenshot[bufIdx + 1] !== next.screenshot[bufIdx + 1]) {

                    this.diffPixels.push(pxIdx);
                }
            }
        }
    }
}

class Profile {
    static BEGIN_PROFILE_REGEX = /^Begin RSP profile$/;
    static END_PROFILE_REGEX = /^End RSP profile$/;

    constructor(symbolMap, screenshotDir) {
        const now = new Date();
        this.name = `profile-` +
            `${now.getFullYear() % 100}` +
            `${now.getMonth() + 1}`.padStart(2, "0") +
            `${now.getDate()}`.padStart(2, "0") + `-` +
            `${now.getHours()}`.padStart(2, "0") +
            `${now.getMinutes()}`.padStart(2, "0") +
            `${now.getSeconds()}`.padStart(2, "0") +
            `${now.getMilliseconds()}`.padStart(3, "0");
        this.isFinished = false;

        this._displayList = new DisplayList(symbolMap);
        this._screenshotDir = screenshotDir;
        this._commandInfo = [];
    }

    static isProfileStartLine(line) {
        return Profile.BEGIN_PROFILE_REGEX.test(line);
    }

    static isProfileEndLine(line) {
        return Profile.END_PROFILE_REGEX.test(line);
    }

    _finish() {
        for (let i = 0; i < this._commandInfo.length; ++i) {
            const current = this._commandInfo[i];
            current.finish();

            const prev = this._commandInfo[i - 1];
            prev?.computeDiff(current);
        }

        // The last command is always a pipe sync we don't care about
        this._commandInfo.pop();
        this.isFinished = true;
    }

    tryUpdate(line) {
        if (Profile.isProfileEndLine(line)) {
            this._finish();
            return true;
        }

        const info = CommandInfo.parse(line);
        if (info) {
            if (info.commandIndex === this._commandInfo.length) {
                this._commandInfo.push(info);
            } else if (info.commandIndex === this._commandInfo.length - 1) {
                this._commandInfo.at(-1).accumulate(info);
            } else {
                throw new Error("Sample indices must be sequential");
            }

            return true;
        }

        if (this._commandInfo.at(-1)?.tryUpdate(line, this._screenshotDir)) {
            return true;
        }

        return this._displayList.tryUpdate(line);
    }

    formatStats(sort=true) {
        const info = this._commandInfo.slice();
        if (sort) {
            info.sort((a, b) => b.runTimeMs - a.runTimeMs);
        }

        const columnPadding = [20, 6, 0];
        const pad = (s, i) => s.toString().padEnd(columnPadding[i]);

        return [
            ["Run time (ms)", "Idx", "Command"],
            ["-------------", "---", "-------"],
            ...info.map(c => [
                c.runTimeMs.toFixed(16),
                c.commandIndex,
                this._displayList.getCommandName(c)
            ])
        ].map(line => line.map(pad).join(" ")).join("\n");
    }

    write(outputDir, sort=true) {
        const fileNameBase = path.join(outputDir, this.name);
        fs.writeFileSync(`${fileNameBase}.txt`, this.formatStats(sort));

        const heatmap = new Heatmap();
        for (const info of this._commandInfo) {
            heatmap.update(info.diffPixels, info.runTimeMs);
        }

        if (!heatmap.isEmpty()) {
            heatmap.writeBitmap(`${fileNameBase}.bmp`);
        }
    }
}

// Main
function printHelp(options) {
    const valueName = (name, arg) => arg.type === "string" ? ` ${name.replace(/-/g, "_").toUpperCase()}` : "";
    const usageLine = [];
    const argLines = [];
    let argLinePad = 0;

    for (const [name, arg] of Object.entries(options)) {
        let argUsage = `-${arg.short}${valueName(name, arg)}`;
        if (!arg.required) {
            argUsage = `[${argUsage}]`;
        }
        usageLine.push(argUsage);

        argLines.push([`  -${arg.short}, --${name}${valueName(name, arg)}`, arg.description]);
        argLinePad = Math.max(argLinePad, argLines.at(-1)[0].length);
    }
    argLinePad += 4;

    console.log("Generates frame performance data from RSP profiler output");
    console.log(`Usage: ${path.basename(process.argv[1])} ${usageLine.join(" ")}`);
    console.log();
    console.log(`Arguments:\n${argLines.map(a => `${a[0].padEnd(argLinePad)}${a[1]}`).join("\n")}`);
}

function parseArgs(options) {
    const { values } = util.parseArgs({ options, allowPositionals: false });

    if (values.help || Object.entries(options).some(([k, v]) => !values[k] && v.required)) {
        printHelp(options);
        process.exit(1);
    }

    const camelCase = (s) => s.replace(/-(\w)/g, (_, c) => c.toUpperCase());
    return Object.fromEntries(Object.entries(values).map(([k, v]) => [camelCase(k), v]));
}

const options = {
    "debug-log-file": {
        short: "d", type: "string", required: true,
        description: "File containing debug console output"
    },
    "symbol-map-file": {
        short: "m", type: "string",
        description: "Linker-generated file containing symbol names and addresses"
    },
    "screenshot-dir": {
        short: "s", type: "string", default: process.cwd(),
        description: "Directory containing screenshot files (defaults to current directory)"
    },
    "output-dir": {
        short: "o", type: "string", default: process.cwd(),
        description: "Directory to output results to (defaults to current directory)"
    },
    "unsorted": {
        short: "u", type: "boolean",
        description: "Output the summary without sorting by command run time"
    },
    "help": {
        short: "h", type: "boolean",
        description: "Print this help text and exit"
    }
};

const { symbolMapFile, debugLogFile, screenshotDir, outputDir, unsorted } = parseArgs(options);

const symbolMap = new SymbolMap(symbolMapFile);
let currentProfile = null;

const debugLogLines = fs.readFileSync(debugLogFile, "utf-8").split('\n');
for (let i = 0; i < debugLogLines.length; ++i) {
    const line = debugLogLines[i].trim();

    if (Profile.isProfileStartLine(line)) {
        currentProfile = new Profile(symbolMap, screenshotDir);
    }

    if (currentProfile?.tryUpdate(line)) {
        if (currentProfile.isFinished) {
            currentProfile.write(outputDir, sort=!unsorted);
            console.log(`Wrote ${currentProfile.name} to ${outputDir}`);

            currentProfile = null;
        }

        continue;
    }

    symbolMap.tryUpdate(line);
}
