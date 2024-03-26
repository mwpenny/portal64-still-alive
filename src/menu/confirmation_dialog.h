#ifndef __MENU_CONFIRMATION_DIALOG_H__
#define __MENU_CONFIRMATION_DIALOG_H__

#include "./menu.h"
#include "../graphics/graphics.h"

typedef void (*ConfirmationDialogCallback)(void* data, int isConfirmed);

struct ConfirmationDialogParams {
    char* title;
    char* message;
    char* confirmLabel;
    char* cancelLabel;
    u8 isTranslucent;
    ConfirmationDialogCallback closeCallback;
    void* callbackData;
};

struct ConfirmationDialog {
    Gfx* menuOutline;
    struct PrerenderedText* titleText;
    struct PrerenderedText* messageText;
    struct MenuButton confirmButton;
    struct MenuButton cancelButton;
    struct MenuButton* selectedButton;
    ConfirmationDialogCallback closeCallback;
    void* callbackData;
    u8 opacity;
    u8 isShown;
};

void confirmationDialogInit(struct ConfirmationDialog* confirmationDialog);
void confirmationDialogShow(struct ConfirmationDialog* confirmationDialog, struct ConfirmationDialogParams* params);
enum InputCapture confirmationDialogUpdate(struct ConfirmationDialog* confirmationDialog);
void confirmationDialogRender(struct ConfirmationDialog* confirmationDialog, struct RenderState* renderState);

#endif
