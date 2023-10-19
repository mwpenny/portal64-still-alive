#include "./player_rumble_clips.h"

unsigned char gPlayerDieRumbleData[] = {
    0xFF, 0xE9, 0x9E, 0xFF, 0xFF, 0xE9
};

struct RumblePakWave gPlayerDieRumbleWave = {
    .samples = gPlayerDieRumbleData,
    .sampleCount = 24,
    .samplesPerTick = 1 << 5,
};