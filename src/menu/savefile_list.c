#include "./savefile_list.h"

#include "../font/dejavusans.h"
#include "../controls/controller.h"
#include "../util/rom.h"
#include "../graphics/image.h"
#include "../audio/soundplayer.h"
#include "./text_manipulation.h"
#include "./controls.h"
#include "./translations.h"
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
    if (savefileListSlot->testChamberText) {
        menuFreePrerenderedDeferred(savefileListSlot->testChamberText);
        savefileListSlot->testChamberText = NULL;
    }

    if (savefileListSlot->gameId) {
        menuFreePrerenderedDeferred(savefileListSlot->testChamberText);
        savefileListSlot->gameId = NULL;
    }

    char message[64];
    textManipTestChamberMessage(message, savefileInfo->testchamberDisplayNumber);
    savefileListSlot->testChamberText = menuBuildPrerenderedText(&gDejaVuSansFont, message, x + BORDER_WIDTH + 8, y, 120);

    if (savefileInfo->savefileName) {
        strcpy(message, savefileInfo->savefileName);
    } else {
        textManipSubjectMessage(message, gSaveData.saveSlotMetadata[savefileInfo->slotIndex].testSubjectNumber);
    }

    savefileListSlot->gameId = menuBuildPrerenderedText(&gDejaVuSansFont, message, x + BORDER_WIDTH + 8, y + savefileListSlot->testChamberText->height + 4, 120);

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
    savefileListSlot->testChamberText = NULL;
    savefileListSlot->gameId = NULL;
    savefileListSlot->border = menuBuildSolidBorder(
        x, y, BORDER_WIDTH, BORDER_HEIGHT,
        x + BORDER_THICKNESS, y + BORDER_THICKNESS, 82, 48
    );

    savefileListSlot->x = x;
    savefileListSlot->y = y;
    savefileListSlot->imageData = malloc(THUMBNAIL_IMAGE_SIZE);
    savefileListSlot->slotIndex = -1;
}

#define LOAD_GAME_LEFT       40
#define LOAD_GAME_TOP        15

#define FILE_OFFSET_X        16
#define FILE_OFFSET_Y        28

#define CONTROLS_HEIGHT      10
#define CONTROL_TEXT_PADDING 12
#define CONTROL_TEXT_MARGIN  2

#define CONTENT_X       (LOAD_GAME_LEFT + 8)
#define CONTENT_Y       (LOAD_GAME_TOP + FILE_OFFSET_Y - 8)
#define CONTENT_WIDTH   (SCREEN_WD - CONTENT_X * 2)
#define CONTENT_HEIGHT  (SCREEN_HT - CONTENT_Y - LOAD_GAME_TOP - CONTROLS_HEIGHT - 8)

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
    savefileList->deleteText = NULL;
    savefileList->confirmText = NULL;

    savefileList->numberOfSaves = 3;
    savefileList->scrollOffset = 0;

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        savefileListSlotInit(
            &savefileList->slots[i], 
            LOAD_GAME_LEFT + FILE_OFFSET_X, 
            LOAD_GAME_TOP + i * ROW_HEIGHT + FILE_OFFSET_Y
        );
    }

    confirmationDialogInit(&savefileList->confirmationDialog);
}

void savefileUseList(struct SavefileListMenu* savefileList, char* title, char* confirmLabel, struct SavefileInfo* savefileInfo, int slotCount) {
    if (savefileList->savefileListTitleText) {
        prerenderedTextFree(savefileList->savefileListTitleText);
    }
    if (savefileList->deleteText) {
        prerenderedTextFree(savefileList->deleteText);
    }
    if (savefileList->confirmText) {
        prerenderedTextFree(savefileList->confirmText);
    }
    
    savefileList->savefileListTitleText = menuBuildPrerenderedText(
        &gDejaVuSansFont,
        title,
        CONTENT_X,
        LOAD_GAME_TOP + 4,
        SCREEN_WD
    );
    savefileList->deleteText = menuBuildPrerenderedText(&gDejaVuSansFont,
        translationsGet(GAMEUI_DELETE),
        CONTENT_X + CONTROL_TEXT_PADDING,
        CONTENT_Y + CONTENT_HEIGHT + CONTROL_TEXT_MARGIN,
        120
    );
    savefileList->confirmText = menuBuildPrerenderedText(
        &gDejaVuSansFont,
        confirmLabel,
        0,
        CONTENT_Y + CONTENT_HEIGHT + CONTROL_TEXT_MARGIN,
        120
    );
    prerenderedTextRelocate(
        savefileList->confirmText,
        CONTENT_X + CONTENT_WIDTH - savefileList->confirmText->width - 1,
        savefileList->confirmText->y
    );

    for (int i = 0; i < slotCount; ++i) {
        savefileList->savefileInfo[i] = savefileInfo[i];
    }

    savefileList->selectedSave = 0;
    savefileList->numberOfSaves = slotCount;
    
    savefileListMenuSetScroll(savefileList, 0);
}

enum InputCapture savefileListUpdate(struct SavefileListMenu* savefileList) {
    if (savefileList->confirmationDialog.isShown) {
        return confirmationDialogUpdate(&savefileList->confirmationDialog);
    }

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return InputCaptureExit;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        savefileList->selectedSave = savefileList->selectedSave + 1;

        if (savefileList->selectedSave == savefileList->numberOfSaves) {
            savefileList->selectedSave = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        savefileList->selectedSave = savefileList->selectedSave - 1;

        if (savefileList->selectedSave < 0) {
            savefileList->selectedSave = savefileList->numberOfSaves - 1;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    int selectTop = SCROLLED_ROW_Y(savefileList->selectedSave, savefileList->scrollOffset) - 8;
    int selectBottom = selectTop + ROW_HEIGHT + 16;

    if (selectBottom > CONTENT_Y + CONTENT_HEIGHT) {
        savefileListMenuSetScroll(savefileList, savefileList->scrollOffset + CONTENT_Y + CONTENT_HEIGHT - selectBottom);
    }

    if (selectTop < CONTENT_Y) {
        savefileListMenuSetScroll(savefileList, savefileList->scrollOffset + CONTENT_Y - selectTop);
    }

    return InputCapturePass;
}

static void savefileListRenderControls(struct SavefileListMenu* savefileList, struct RenderState* renderState) {
    if (savefileList->numberOfSaves == 0) {
        return;
    }

    struct SavefileInfo* selectedSave = &savefileList->savefileInfo[savefileList->selectedSave];
    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);

    if (savefileList->confirmText) {
        controlsRenderButtonIcon(
            ControllerActionSourceAButton,
            savefileList->confirmText->x - CONTROL_TEXT_PADDING - 2,
            savefileList->confirmText->y,
            renderState
        );
        prerenderedBatchAdd(batch, savefileList->confirmText, NULL);
    }
    if (savefileList->deleteText && !selectedSave->isFree) {
        controlsRenderButtonIcon(
            ControllerActionSourceZTrig,
            savefileList->deleteText->x - CONTROL_TEXT_PADDING - 1,
            savefileList->deleteText->y,
            renderState
        );
        prerenderedBatchAdd(batch, savefileList->deleteText, NULL);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
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

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    if (savefileList->savefileListTitleText) {
        prerenderedBatchAdd(batch, savefileList->savefileListTitleText, NULL);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    savefileListRenderControls(savefileList, renderState);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    batch = prerenderedBatchStart();

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        struct Coloru8* color = savefileList->indexOffset + i == savefileList->selectedSave ? &gSelectionOrange : &gColorWhite;

        prerenderedBatchAdd(batch, slot->testChamberText, color);

        if (slot->gameId) {
            prerenderedBatchAdd(batch, slot->gameId, color);
        }
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);


    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

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

    if (savefileList->confirmationDialog.isShown) {
        confirmationDialogRender(&savefileList->confirmationDialog, renderState);
    }

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}

int savefileGetSlot(struct SavefileListMenu* savefileList) {
    return savefileList->savefileInfo[savefileList->selectedSave].slotIndex;
}

void savefileListConfirmDeletion(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData) {
    struct ConfirmationDialogParams dialogParams = {
        translationsGet(GAMEUI_CONFIRMDELETESAVEGAME_TITLE),
        translationsGet(GAMEUI_CONFIRMDELETESAVEGAME_INFO),
        translationsGet(GAMEUI_CONFIRMDELETESAVEGAME_OK),
        translationsGet(GAMEUI_CANCEL),
        0,
        callback,
        callbackData
    };

    confirmationDialogShow(&savefileList->confirmationDialog, &dialogParams);
}

void savefileListConfirmOverwrite(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData) {
    struct ConfirmationDialogParams dialogParams = {
        translationsGet(GAMEUI_CONFIRMOVERWRITESAVEGAME_TITLE),
        translationsGet(GAMEUI_CONFIRMOVERWRITESAVEGAME_INFO),
        translationsGet(GAMEUI_CONFIRMOVERWRITESAVEGAME_OK),
        translationsGet(GAMEUI_CANCEL),
        0,
        callback,
        callbackData
    };

    confirmationDialogShow(&savefileList->confirmationDialog, &dialogParams);
}
