#include "text_manipulation.h"

#include <ultra64.h>

#include "strings/translations.h"
#include "util/string.h"

#include "codegen/src/strings/strings.h"

void textManipTestChamberMessage(char* result, int testChamber) {
    strCopy(result, translationsGet(PORTAL_CHAPTER1_TITLE));
    int len = strLength(result);

    // this is dumb, but it works
    result[len - 1] += testChamber % 10;
    result[len - 2] += testChamber / 10;
}

void textManipSubjectMessage(char* result, int subjectIndex) {
    sprintf(result, "%s %02d", translationsGet(GAMEUI_PLAYERNAME), subjectIndex);
}
