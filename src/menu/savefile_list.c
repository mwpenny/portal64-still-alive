#include "./savefile_list.h"

#include "../font/dejavusans.h"
#include "../controls/controller.h"
#include "../util/rom.h"
#include "../graphics/image.h"
#include "../audio/soundplayer.h"
#include <string.h>

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define SAVE_SLOT_RENDER_W  (SAVE_SLOT_IMAGE_W * 2)
#define SAVE_SLOT_RENDER_H  (SAVE_SLOT_IMAGE_H * 2)

#define BORDER_THICKNESS    5
#define BORDER_WIDTH        (SAVE_SLOT_RENDER_W + BORDER_THICKNESS * 2)
#define BORDER_HEIGHT       (SAVE_SLOT_RENDER_H + BORDER_THICKNESS * 2)

#define ROW_HEIGHT      (BORDER_HEIGHT + 8)

void savefileListSlotUseInfo(struct SavefileListSlot* savefileListSlot, struct SavefileInfo* savefileInfo, int x, int y) {
    char message[16];
    sprintf(message, "Testchamber %02d", savefileInfo->testchamberIndex);
    fontRender(&gDejaVuSansFont, message, x + BORDER_WIDTH + 8, y, savefileListSlot->testChamberText);

    if (savefileListSlot->gameId) {
        free(savefileListSlot->gameId);
    }

    if (savefileInfo->savefileName) {
        strcpy(message, savefileInfo->savefileName);
    } else {
        sprintf(message, "Subject %02d", gSaveData.saveSlotMetadata[savefileInfo->slotIndex].testSubjectNumber);
    }

    savefileListSlot->gameId = menuBuildText(&gDejaVuSansFont, message, x + BORDER_WIDTH + 8, y + 16);

    menuRerenderSolidBorder(
        x, y, 
        BORDER_WIDTH, BORDER_HEIGHT,
        x + BORDER_THICKNESS, y + BORDER_THICKNESS, 
        SAVE_SLOT_IMAGE_W * 2, SAVE_SLOT_IMAGE_H * 2,
        savefileListSlot->border
    );
    savefileListSlot->slotIndex = savefileInfo->slotIndex;

    savefileLoadScreenshot(savefileListSlot->imageData, savefileInfo->screenshot);
    savefileListSlot->x = x;
    savefileListSlot->y = y;
}

void savefileListSlotInit(struct SavefileListSlot* savefileListSlot, int x, int y) {
    savefileListSlot->testChamberText = menuBuildText(&gDejaVuSansFont, "Testchamber 00", x + BORDER_WIDTH + 8, y);
    savefileListSlot->gameId = NULL;
    savefileListSlot->border = menuBuildSolidBorder(
        x, y, BORDER_WIDTH, BORDER_HEIGHT,
        x + BORDER_THICKNESS, y + BORDER_THICKNESS, 82, 48
    );

    savefileListSlot->x = x;
    savefileListSlot->y = y;
    savefileListSlot->imageData = malloc(THUMBANIL_IMAGE_SIZE);
    savefileListSlot->slotIndex = -1;
}

#define LOAD_GAME_LEFT       40
#define LOAD_GAME_TOP        20

#define FILE_OFFSET_X       16
#define FILE_OFFSET_Y       28

#define CONTENT_X       (LOAD_GAME_LEFT + 8)
#define CONTENT_Y       (LOAD_GAME_TOP + FILE_OFFSET_Y - 8)
#define CONTENT_WIDTH   (SCREEN_WD - CONTENT_X * 2)
#define CONTENT_HEIGHT  (SCREEN_HT - CONTENT_Y - LOAD_GAME_TOP - 8)

#define SCROLLED_ROW_Y(rowIndex, scrollOffset) (LOAD_GAME_TOP + (rowIndex) * ROW_HEIGHT + FILE_OFFSET_Y + (scrollOffset))

void savefileListMenuSetScroll(struct SavefileListMenu* savefileList, int amount) {
    int minScroll = CONTENT_HEIGHT - ROW_HEIGHT * savefileList->numberOfSaves - 8;

    if (amount < minScroll) {
        amount = minScroll;
    }

    if (amount > 0) {
        amount = 0;
    }

    savefileList->scrollOffset = amount;
    savefileList->indexOffset = -amount / ROW_HEIGHT;

    int i;

    for (i = 0; i + savefileList->indexOffset < savefileList->numberOfSaves && i < MAX_VISIBLE_SLOTS; ++i) {
        savefileListSlotUseInfo(
            &savefileList->slots[i], 
            &savefileList->savefileInfo[i + savefileList->indexOffset],
            LOAD_GAME_LEFT + FILE_OFFSET_X,
            SCROLLED_ROW_Y(i + savefileList->indexOffset, savefileList->scrollOffset)
        );
    }

    for (; i < MAX_VISIBLE_SLOTS; ++i) {
        savefileList->slots[i].slotIndex = -1;
    }
}

void savefileListMenuInit(struct SavefileListMenu* savefileList) {
    savefileList->menuOutline = menuBuildBorder(LOAD_GAME_LEFT, LOAD_GAME_TOP, SCREEN_WD - LOAD_GAME_LEFT * 2, SCREEN_HT - LOAD_GAME_TOP * 2);
    savefileList->savefileListTitleText = NULL;

    savefileList->numberOfSaves = 3;
    savefileList->scrollOffset = 0;

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        savefileListSlotInit(
            &savefileList->slots[i], 
            LOAD_GAME_LEFT + FILE_OFFSET_X, 
            LOAD_GAME_TOP + i * ROW_HEIGHT + FILE_OFFSET_Y
        );
    }
}

void savefileUseList(struct SavefileListMenu* savefileList, char* title, struct SavefileInfo* savefileInfo, int slotCount) {
    if (savefileList->savefileListTitleText) {
        free(savefileList->savefileListTitleText);
    }
    
    savefileList->savefileListTitleText = menuBuildText(&gDejaVuSansFont, title, 48, LOAD_GAME_TOP + 4);

    for (int i = 0; i < slotCount; ++i) {
        savefileList->savefileInfo[i] = savefileInfo[i];
    }

    savefileList->selectedSave = 0;
    savefileList->numberOfSaves = slotCount;
    
    savefileListMenuSetScroll(savefileList, 0);
}

enum MenuDirection savefileListUpdate(struct SavefileListMenu* savefileList) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        savefileList->selectedSave = savefileList->selectedSave + 1;

        if (savefileList->selectedSave == savefileList->numberOfSaves) {
            savefileList->selectedSave = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL);
    }

    if (controllerDir & ControllerDirectionUp) {
        savefileList->selectedSave = savefileList->selectedSave - 1;

        if (savefileList->selectedSave < 0) {
            savefileList->selectedSave = savefileList->numberOfSaves - 1;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL);
    }

    int selectTop = SCROLLED_ROW_Y(savefileList->selectedSave, savefileList->scrollOffset) - 8;
    int selectBottom = selectTop + ROW_HEIGHT + 16;

    if (selectBottom > CONTENT_Y + CONTENT_HEIGHT) {
        savefileListMenuSetScroll(savefileList, savefileList->scrollOffset + CONTENT_Y + CONTENT_HEIGHT - selectBottom);
    }

    if (selectTop < CONTENT_Y) {
        savefileListMenuSetScroll(savefileList, savefileList->scrollOffset + CONTENT_Y - selectTop);
    }

    return MenuDirectionStay;
}

void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, savefileList->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, savefileList->indexOffset + i == savefileList->selectedSave, &gSelectionOrange, &gColorBlack);

        renderStateInlineBranch(renderState, slot->border);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);

    if (savefileList->savefileListTitleText) {
        gSPDisplayList(renderState->dl++, savefileList->savefileListTitleText);
    }

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, savefileList->indexOffset + i == savefileList->selectedSave, &gSelectionOrange, &gColorWhite);

        renderStateInlineBranch(renderState, slot->testChamberText);
        renderStateInlineBranch(renderState, slot->gameId);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[IMAGE_COPY_INDEX]);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        gDPLoadTextureTile(
            renderState->dl++,
            K0_TO_PHYS(slot->imageData),
            G_IM_FMT_RGBA, G_IM_SIZ_16b,
            SAVE_SLOT_IMAGE_W, SAVE_SLOT_IMAGE_H,
            0, 0,
            SAVE_SLOT_IMAGE_W-1, SAVE_SLOT_IMAGE_H-1,
            0,
            G_TX_CLAMP, G_TX_CLAMP,
            G_TX_NOMASK, G_TX_NOMASK,
            G_TX_NOLOD, G_TX_NOLOD
        );
        
        gSPTextureRectangle(
            renderState->dl++,
            (slot->x + BORDER_THICKNESS) << 2, (slot->y + BORDER_THICKNESS) << 2,
            (slot->x + BORDER_THICKNESS + SAVE_SLOT_RENDER_W) << 2, 
            (slot->y + BORDER_THICKNESS + SAVE_SLOT_RENDER_H) << 2,
            G_TX_RENDERTILE, 
            0, 0, 
            (SAVE_SLOT_IMAGE_W << 10) / SAVE_SLOT_RENDER_W, (SAVE_SLOT_IMAGE_H << 10) / SAVE_SLOT_RENDER_H
        );
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[IMAGE_COPY_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}

int savefileGetSlot(struct SavefileListMenu* savefileList) {
    return savefileList->savefileInfo[savefileList->selectedSave].slotIndex;
}