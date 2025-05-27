const fs = require("fs");
const path = require("path");

const NORMALIZATION_RULES = {
    // The Russian localization only has one sound of each type
    // Rename to match the first sounds used in other languages
    "russian": {
        "npc/turret_floor/turret_disabled_1.wav": {
            type: "rename",
            target: "npc/turret_floor/turret_disabled_2.wav"
        },
        "npc/turret_floor/turret_dissolved_1.wav": {
            type: "rename",
            target: "npc/turret_floor/turret_fizzler_1.wav"
        },
        "npc/turret_floor/turret_tipped_1.wav": {
            type: "rename",
            target: "npc/turret_floor/turret_tipped_2.wav"
        }
    }
};

function applyRule(basePath, sourcePath, rule) {
    sourcePath = path.join(basePath, sourcePath);

    switch (rule.type) {
        case "rename":
            if (fs.existsSync(sourcePath)) {
                const destPath = path.join(basePath, rule.target);
                fs.renameSync(sourcePath, destPath);
            }
            break;
        default:
            throw new Error(`Unknown rule type '${rule.type}'`);
    }
}


if (process.argv.length !== 3) {
    console.log('Applies normalization rules to localized audio file names.\n');
    console.log(`Usage: ${process.argv[0]} ${process.argv[1]} GAME_ROOT_DIR`);
    process.exit(1);
}

const gameRootDir = process.argv[2];

for (const [language, rules] of Object.entries(NORMALIZATION_RULES)) {
    const basePath = path.join(gameRootDir, "localized", language, "sound");

    for (const [sourcePath, rule] of Object.entries(rules)) {
        applyRule(basePath, sourcePath, rule);
    }
}
