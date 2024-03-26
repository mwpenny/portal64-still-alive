#include "./confirmation_dialog.h"

#include "../font/dejavusans.h"
#include "../controls/controller.h"
#include "../audio/soundplayer.h"
#include "./translations.h"

#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"
#include "../build/src/audio/subtitles.h"

#define DIALOG_LEFT       40
#define DIALOG_WIDTH      (SCREEN_WD - (DIALOG_LEFT * 2))
#define DIALOG_PADDING    8

#define TEXT_MARGIN       4

#define CONTENT_LEFT      (DIALOG_LEFT + DIALOG_PADDING)
#define CONTENT_WIDTH     (DIALOG_WIDTH - (DIALOG_PADDING * 2))
#define CONTENT_CENTER_X  (CONTENT_LEFT + (CONTENT_WIDTH / 2))
#define CONTENT_MARGIN    20

#define BUTTON_HEIGHT     16
#define BUTTON_MARGIN     8

static struct Coloru8 gDialogColor = {164, 164, 164, 255};

void confirmationDialogInit(struct ConfirmationDialog* confirmationDialog) {
    confirmationDialog->menuOutline = menuBuildBorder(0, 0, 0, 0);
    confirmationDialog->titleText = NULL;
    confirmationDialog->messageText = NULL;
    confirmationDialog->selectedButton = NULL;
    confirmationDialog->closeCallback = NULL;
    confirmationDialog->callbackData = NULL;
    confirmationDialog->opacity = 255;
    confirmationDialog->isShown = 0;

    confirmationDialog->confirmButton = menuBuildButton(
        &gDejaVuSansFont, "",
        0,
        0,
        BUTTON_HEIGHT,
        0
    );
    confirmationDialog->cancelButton = menuBuildButton(
        &gDejaVuSansFont, "",
        0,
        0,
        BUTTON_HEIGHT,
        0
    );
}

static void confirmationDialogLayout(struct ConfirmationDialog* confirmationDialog) {
    int dialogHeight = CONTENT_MARGIN + confirmationDialog->messageText->height + BUTTON_MARGIN + BUTTON_HEIGHT + DIALOG_PADDING;
    int dialogTop = (SCREEN_HT - dialogHeight) / 2;

    menuRerenderBorder(
        DIALOG_LEFT,
        dialogTop,
        DIALOG_WIDTH,
        dialogHeight,
        confirmationDialog->menuOutline
    );
    prerenderedTextRelocate(
        confirmationDialog->titleText,
        CONTENT_LEFT,
        dialogTop + TEXT_MARGIN
    );
    prerenderedTextRelocate(
        confirmationDialog->messageText,
        CONTENT_LEFT,
        dialogTop + CONTENT_MARGIN
    );

    int buttonsWidth = confirmationDialog->confirmButton.w + confirmationDialog->cancelButton.w + BUTTON_MARGIN;
    int buttonsX = CONTENT_CENTER_X - (buttonsWidth / 2);
    int buttonsY = confirmationDialog->messageText->y + confirmationDialog->messageText->height + BUTTON_MARGIN;

    menuRelocateButton(
        &confirmationDialog->confirmButton,
        buttonsX,
        buttonsY,
        0
    );
    menuRelocateButton(
        &confirmationDialog->cancelButton,
        buttonsX + confirmationDialog->confirmButton.w + BUTTON_MARGIN,
        buttonsY,
        0
    );
}

void confirmationDialogShow(struct ConfirmationDialog* confirmationDialog, struct ConfirmationDialogParams* params) {
    if (confirmationDialog->titleText) {
        prerenderedTextFree(confirmationDialog->titleText);
    }
    if (confirmationDialog->messageText) {
        prerenderedTextFree(confirmationDialog->messageText);
    }

    confirmationDialog->titleText = menuBuildPrerenderedText(
        &gDejaVuSansFont, params->title,
        0,
        0,
        SCREEN_WD
    );
    confirmationDialog->messageText = menuBuildPrerenderedText(
        &gDejaVuSansFont, params->message,
        0,
        0,
        CONTENT_WIDTH
    );

    menuRebuildButtonText(
        &confirmationDialog->confirmButton,
        &gDejaVuSansFont,
        params->confirmLabel,
        0
    );
    menuRebuildButtonText(
        &confirmationDialog->cancelButton,
        &gDejaVuSansFont,
        params->cancelLabel,
        0
    );

    confirmationDialogLayout(confirmationDialog);

    confirmationDialog->selectedButton = &confirmationDialog->cancelButton;
    confirmationDialog->closeCallback = params->closeCallback;
    confirmationDialog->callbackData = params->callbackData;
    confirmationDialog->opacity = params->isTranslucent ? 128 : 255;
    confirmationDialog->isShown = 1;
}

static void confirmationDialogClose(struct ConfirmationDialog* confirmationDialog, int isConfirmed) {
    ConfirmationDialogCallback callback = confirmationDialog->closeCallback;
    if (callback) {
        callback(confirmationDialog->callbackData, isConfirmed);
    }

    confirmationDialog->isShown = 0;
}

enum InputCapture confirmationDialogUpdate(struct ConfirmationDialog* confirmationDialog) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        confirmationDialogClose(confirmationDialog, 0);
    } else if (controllerGetButtonDown(0, A_BUTTON)) {
        if (confirmationDialog->selectedButton == &confirmationDialog->confirmButton) {
            confirmationDialogClose(confirmationDialog, 1);
        } else {
            confirmationDialogClose(confirmationDialog, 0);
        }

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & (ControllerDirectionLeft | ControllerDirectionRight)) {
        if (confirmationDialog->selectedButton == &confirmationDialog->confirmButton) {
            confirmationDialog->selectedButton = &confirmationDialog->cancelButton;
        } else {
            confirmationDialog->selectedButton = &confirmationDialog->confirmButton;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    return InputCaptureGrab;
}

static void renderButtonText(struct ConfirmationDialog* confirmationDialog, struct MenuButton* button, struct PrerenderedTextBatch* batch) {
    struct Coloru8* color = confirmationDialog->selectedButton == button ? &gColorBlack : &gColorWhite;
    prerenderedBatchAdd(batch, button->text, color);
}

void confirmationDialogRender(struct ConfirmationDialog* confirmationDialog, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gDialogColor.r, gDialogColor.g, gDialogColor.b, confirmationDialog->opacity);
    gSPDisplayList(renderState->dl++, confirmationDialog->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);

    if (confirmationDialog->selectedButton != NULL) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        gDPFillRectangle(
            renderState->dl++,
            confirmationDialog->selectedButton->x,
            confirmationDialog->selectedButton->y,
            confirmationDialog->selectedButton->x + confirmationDialog->selectedButton->w,
            confirmationDialog->selectedButton->y + confirmationDialog->selectedButton->h
        );
    }

    gSPDisplayList(renderState->dl++, confirmationDialog->confirmButton.outline);
    gSPDisplayList(renderState->dl++, confirmationDialog->cancelButton.outline);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    if (confirmationDialog->titleText) {
        prerenderedBatchAdd(batch, confirmationDialog->titleText, NULL);
    }
    if (confirmationDialog->messageText) {
        prerenderedBatchAdd(batch, confirmationDialog->messageText, NULL);
    }

    renderButtonText(confirmationDialog, &confirmationDialog->confirmButton, batch);
    renderButtonText(confirmationDialog, &confirmationDialog->cancelButton, batch);

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}
