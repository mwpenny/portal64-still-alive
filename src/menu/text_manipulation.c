#include "text_manipulation.h"

#include "./translations.h"
#include "../build/src/audio/subtitles.h"
#include <string.h>
#include <ultra64.h>

void textManipTestChamberMessage(char* result, int testChamber) {
    strcpy(result, translationsGet(PORTAL_CHAPTER1_TITLE));
    int len = strlen(result);

    // this is dumb, but it works
    result[len - 1] += testChamber % 10;
    result[len - 2] += testChamber / 10;
}

void textManipSubjectMessage(char* result, int subjectIndex) {
    sprintf(result, "%s %02d", translationsGet(GAMEUI_PLAYERNAME), subjectIndex);
}