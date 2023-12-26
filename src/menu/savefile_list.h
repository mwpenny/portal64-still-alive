#ifndef __MENU_SAVEFILE_LIST_H__
#define __MENU_SAVEFILE_LIST_H__

#include "./menu.h"
#include "../font/font.h"
#include "../graphics/graphics.h"
#include "../savefile/savefile.h"
#include "./new_game_menu.h"
#include "./confirmation_dialog.h"

struct SavefileInfo {
    short slotIndex;
    short testchamberDisplayNumber;
    char* savefileName;
    u16* screenshot;
    int isFree;
};

struct SavefileListSlot {
    struct PrerenderedText* testChamberText;
    Gfx* border;
    struct PrerenderedText* gameId;
    short x, y;
    short slotIndex;
    void* imageData;
};

#define MAX_VISIBLE_SLOTS  4

struct SavefileListMenu {
    Gfx* menuOutline;
    struct PrerenderedText* savefileListTitleText;
    struct PrerenderedText* deleteText;
    struct PrerenderedText* confirmText;
    struct ConfirmationDialog confirmationDialog;
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SavefileListSlot slots[MAX_VISIBLE_SLOTS];
    short numberOfSaves;
    short scrollOffset;
    short indexOffset;
    short selectedSave;
};

void savefileListMenuInit(struct SavefileListMenu* savefileList);
void savefileUseList(struct SavefileListMenu* savefileList, char* title, char* confirmLabel, struct SavefileInfo* savefileInfo, int slotCount);
enum InputCapture savefileListUpdate(struct SavefileListMenu* savefileList);
void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task);
int savefileGetSlot(struct SavefileListMenu* savefileList);
void savefileListConfirmDeletion(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData);
void savefileListConfirmOverwrite(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData);

#endif