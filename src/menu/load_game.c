#include "load_game.h"

#include "../font/dejavusans.h"
#include "../controls/controller.h"
#include "../util/rom.h"
#include "../graphics/image.h"

#include "../build/assets/materials/ui.h"

#define BORDER_WIDTH    92
#define BORDER_HEIGHT   58

void loadGameSlotUseSlot(struct LoadGameSlot* loadGameSlot, struct SaveSlotInfo slotInfo) {
    loadGameSlot->saveSlot = slotInfo.saveSlot;
    struct Chapter* chapter = chapterFindForChamber(slotInfo.testChamber);
    romCopy(chapter->imageData, loadGameSlot->imageData, CHAPTER_IMAGE_SIZE);
}

void loadGameSlotInit(struct LoadGameSlot* loadGameSlot, int x, int y) {
    loadGameSlot->testChamberText = menuBuildText(&gDejaVuSansFont, "Testchamber 00", x + BORDER_WIDTH + 8, y);
    loadGameSlot->border = menuBuildSolidBorder(
        x, y, BORDER_WIDTH, BORDER_HEIGHT,
        x + 5, y + 5, 82, 48
    );

    loadGameSlot->x = x;
    loadGameSlot->y = y;
    loadGameSlot->imageData = malloc(CHAPTER_IMAGE_SIZE);
    loadGameSlot->saveSlot = 0;
}

#define LOAD_GAME_LEFT       40
#define LOAD_GAME_TOP        20

#define FILE_OFFSET_X       16
#define FILE_OFFSET_Y       28

#define CONTENT_X       (LOAD_GAME_LEFT + 8)
#define CONTENT_Y       (LOAD_GAME_TOP + FILE_OFFSET_Y - 8)
#define CONTENT_WIDTH   (SCREEN_WD - CONTENT_X * 2)
#define CONTENT_HEIGHT  (SCREEN_HT - CONTENT_Y - LOAD_GAME_TOP - 8)

void loadGameMenuReread(struct LoadGameMenu* loadGame) {
    loadGame->numberOfSaves = savefileListSaves(loadGame->slotInfo);
    loadGame->scrollOffset = 0;
    loadGame->selectedSave = 0;

    loadGame->slotInfo[0].saveSlot = 0;
    loadGame->slotInfo[0].testChamber = 0;
    loadGame->numberOfSaves = 1;

    int i;

    for (i = 0; i < loadGame->numberOfSaves && i < MAX_VISIBLE_SLOTS; ++i) {
        loadGameSlotUseSlot(&loadGame->slots[i], loadGame->slotInfo[i]);
    }

    for (; i < MAX_VISIBLE_SLOTS; ++i) {
        loadGame->slots[i].saveSlot = -1;
    }
}

void loadGameMenuInit(struct LoadGameMenu* loadGame) {
    loadGame->menuOutline = menuBuildBorder(LOAD_GAME_LEFT, LOAD_GAME_TOP, SCREEN_WD - LOAD_GAME_LEFT * 2, SCREEN_HT - LOAD_GAME_TOP * 2);
    loadGame->loadGameText = menuBuildText(&gDejaVuSansFont, "LOAD GAME", 48, LOAD_GAME_TOP + 4);

    loadGame->numberOfSaves = 3;
    loadGame->scrollOffset = 0;

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        loadGameSlotInit(
            &loadGame->slots[i], 
            LOAD_GAME_LEFT + FILE_OFFSET_X, 
            LOAD_GAME_TOP + i * (BORDER_HEIGHT + 8) + FILE_OFFSET_Y
        );
    }

    loadGameMenuReread(loadGame);
}

enum MenuDirection loadGameUpdate(struct LoadGameMenu* loadGame) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }


    return MenuDirectionStay;
}

void loadGameRender(struct LoadGameMenu* loadGame, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, loadGame->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct LoadGameSlot* slot = &loadGame->slots[i];

        if (slot->saveSlot < 0) {
            continue;
        }

        menuSetRenderColor(renderState, loadGame->scrollOffset + i == loadGame->selectedSave, &gSelectionOrange, &gColorBlack);

        gSPDisplayList(renderState->dl++, slot->border);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);

    gSPDisplayList(renderState->dl++, loadGame->loadGameText);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct LoadGameSlot* slot = &loadGame->slots[i];

        if (slot->saveSlot < 0) {
            continue;
        }

        menuSetRenderColor(renderState, loadGame->scrollOffset + i == loadGame->selectedSave, &gSelectionOrange, &gColorWhite);

        gSPDisplayList(renderState->dl++, slot->testChamberText);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct LoadGameSlot* slot = &loadGame->slots[i];

        if (slot->saveSlot < 0) {
            continue;
        }
        
        graphicsCopyImage(
            renderState,
            slot->imageData,
            CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
            0, 0,
            slot->x + 5, slot->y + 5,
            CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
            gColorWhite
        );
    }

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}
