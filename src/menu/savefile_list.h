#ifndef __MENU_SAVEFILE_LIST_H__
#define __MENU_SAVEFILE_LIST_H__

#include "./menu.h"
#include "../graphics/graphics.h"
#include "../savefile/savefile.h"
#include "./new_game_menu.h"

struct SavefileInfo {
    short slotIndex;
    short testchamberIndex;
    char* savefileName;
    u16* screenshot;
};

struct SavefileListSlot {
    Gfx* testChamberText;
    Gfx* border;
    Gfx* gameId;
    short x, y;
    short slotIndex;
    void* imageData;
};

#define MAX_VISIBLE_SLOTS  4

struct SavefileListMenu {
    Gfx* menuOutline;
    Gfx* savefileListTitleText;
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SavefileListSlot slots[MAX_VISIBLE_SLOTS];
    short numberOfSaves;
    short scrollOffset;
    short indexOffset;
    short selectedSave;
};

void savefileListMenuInit(struct SavefileListMenu* savefileList);
void savefileUseList(struct SavefileListMenu* savefileList, char* title, struct SavefileInfo* savefileInfo, int slotCount);
enum MenuDirection savefileListUpdate(struct SavefileListMenu* savefileList);
void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task);
int savefileGetSlot(struct SavefileListMenu* savefileList);

#endif