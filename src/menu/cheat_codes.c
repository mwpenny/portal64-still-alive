#include "cheat_codes.h"

#include "audio/soundplayer.h"
#include "levels/levels.h"
#include "savefile/savefile.h"
#include "scene/scene.h"

#include "codegen/assets/audio/clips.h"

struct CheatCodePattern gCheatCodes[CheatCodeCount] = {
    [CheatCodeUnlockGun] = {
        {'u', 'u', 'd', 'd', 'l', 'r', 'l', 'r'},
        SOUNDS_01_PART1_GET_PORTAL_GUN_1,
    },
    [CheatCodeHighJump] = {
        {'u', 'd', 'u', 'd', 'u', 'd', 'r', 'r'},
        SOUNDS_BUTTONCLICKRELEASE,
    },
    [CheatCodeInvincibility] = {
        {'u', 'd', 'u', 'd', 'u', 'd', 'l', 'l'},
        SOUNDS_BUTTONCLICKRELEASE,
    },
    [CheatCodeNoAttack] = {
        {'u', 'd', 'u', 'd', 'u', 'd', 'd', 'd'},
        SOUNDS_BUTTONCLICKRELEASE,
    },
    [CheatCodeAllLevels] = {
        {'u', 'r', 'd', 'l', 'u', 'r', 'd', 'l'},
        SOUNDS_BUTTONCLICKRELEASE,
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
            portalGunDraw(&gScene.portalGun);
            break;
        case CheatCodeHighJump:
            playerToggleJumpImpulse(&gScene.player, 6.5f);
            break;
        case CheatCodeInvincibility:
            playerToggleInvincibility(&gScene.player);
            break;
        case CheatCodeNoAttack:
            playerToggleCollisionLayers(&gScene.player, COLLISION_LAYERS_BLOCK_TURRET_SIGHT);
            break;
        case CheatCodeAllLevels:
            savefileMarkChapterProgress(levelCount() - 1);
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
                soundPlayerPlay(gCheatCodes[i].soundId, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
                gCheatProgress[i] = 0;
            }
        } else if (gCheatCodes[i].pattern[0] == dir) {
            gCheatProgress[i] = 1;
        } else {
            gCheatProgress[i] = 0;
        }
    }
}