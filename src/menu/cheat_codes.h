#ifndef __MENU_CHEAT_CODES_H__
#define __MENU_CHEAT_CODES_H__

#define CHEAT_CODE_LENGTH   8

enum CheatCode {
    CheatCodeUnlockGun,
    CheatCodeCount,
};

enum CheatCodeDir {
    CheatCodeDirUp = 'u',
    CheatCodeDirRight = 'r',
    CheatCodeDirDown = 'd',
    CheatCodeDirLeft = 'l',
};

struct CheatCodePattern {
    char pattern[CHEAT_CODE_LENGTH];
    short soundId;
};

void cheatCodeReset();
void cheatCodeEnterDirection(enum CheatCodeDir dir);

#endif