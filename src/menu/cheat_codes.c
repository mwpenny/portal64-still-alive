#include "cheat_codes.h"

#include "../build/src/audio/clips.h"
#include "../audio/soundplayer.h"
#include "../scene/scene.h"

struct CheatCodePattern gCheatCodes[CheatCodeCount] = {
    [CheatCodeUnlockGun] = {
        {'u', 'u', 'd', 'd', 'l', 'r', 'l', 'r'},
        SOUNDS_01_PART1_GET_PORTAL_GUN_1,
    },
};

unsigned char gCheatProgress[CheatCodeCount];

void cheatCodeReset() {
    for (int i = 0; i < CheatCodeCount; ++i) {
        gCheatProgress[i] = 0;
    }
}

void cheatCodeApply(enum CheatCode cheat) {
    switch (cheat) {
        case CheatCodeUnlockGun:
            playerGivePortalGun(&gScene.player, PlayerHasFirstPortalGun | PlayerHasSecondPortalGun);
            break;
        case CheatCodeCount:
            break;
    }
}

void cheatCodeEnterDirection(enum CheatCodeDir dir) {
    for (int i = 0; i < CheatCodeCount; ++i) {
        if (gCheatCodes[i].pattern[gCheatProgress[i]] == dir) {
            ++gCheatProgress[i];

            if (gCheatProgress[i] == CHEAT_CODE_LENGTH) {
                cheatCodeApply(i);
                soundPlayerPlay(gCheatCodes[i].soundId, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
                gCheatProgress[i] = 0;
            }
        } else if (gCheatCodes[i].pattern[0] == dir) {
            gCheatProgress[i] = 1;
        } else {
            gCheatProgress[i] = 0;
        }
    }
}