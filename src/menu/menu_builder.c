#include "menu_builder.h"

#include "../util/memory.h"
#include "../controls/controller.h"
#include "./translations.h"
#include "../audio/soundplayer.h"
#include "../util/time.h"
#include "../math/mathf.h"

#include "../font/dejavusans.h"
#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#include <stdbool.h>

#define MENU_WIDTH 252
#define TEXTHEIGHT 12
#define PADDING_X 2
#define TABWIDTH 232

void textMenuItemInit(struct MenuBuilderElement* element) {
    char* message = element->params->params.text.message;

    if (!message) {
        message = translationsGet(element->params->params.text.messageId);
    }

    element->data = menuBuildPrerenderedText(
        element->params->params.text.font, 
        message,
        element->params->x,
        element->params->y,
        SCREEN_WD
    );
}

void textMenuItemRebuildText(struct MenuBuilderElement* element) {
    prerenderedTextFree(element->data);
    textMenuItemInit(element);
}

void textMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    if (textBatch) {
        prerenderedBatchAdd(textBatch, element->data, selection == element->selectionIndex ? &gColorBlack : &gColorWhite);
        bool isTextPositionedOnFarLeft = (element->params->x < (int)(MENU_WIDTH / 2)) ? true : false;

        if ((selection == element->selectionIndex) && isTextPositionedOnFarLeft){
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
            gDPFillRectangle(
                renderState->dl++, 
                element->params->x - PADDING_X, 
                element->params->y, 
                element->params->x + TABWIDTH + PADDING_X, 
                element->params->y + TEXTHEIGHT
            );
        }
    }
}

void checkboxMenuItemInit(struct MenuBuilderElement* element) {
    struct MenuCheckbox* checkbox = malloc(sizeof(struct MenuCheckbox));
    *checkbox = menuBuildCheckbox(
        element->params->params.checkbox.font,
        translationsGet(element->params->params.checkbox.messageId),
        element->params->x,
        element->params->y
    );
    element->data = checkbox;
}

enum InputCapture checkboxMenuItemUpdate(struct MenuBuilderElement* element, MenuActionCalback actionCallback, void* data) {
    if (controllerGetButtonDown(0, A_BUTTON)) {
        struct MenuCheckbox* checkbox = (struct MenuCheckbox*)element->data;

        checkbox->checked = !checkbox->checked;

        struct MenuAction action;
        action.type = MenuElementTypeCheckbox;
        action.state.checkbox.isChecked = checkbox->checked;
        actionCallback(data, element->selectionIndex, &action);
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll); 
    }

    return InputCapturePass;
}

void checkboxMenuItemRebuildText(struct MenuBuilderElement* element) {
    struct MenuCheckbox* checkbox = (struct MenuCheckbox*)element->data;

    prerenderedTextFree(checkbox->prerenderedText);
    checkbox->prerenderedText = menuBuildPrerenderedText(
        element->params->params.checkbox.font, 
        translationsGet(element->params->params.checkbox.messageId),
        element->params->x + CHECKBOX_SIZE + 6,
        element->params->y,
        SCREEN_WD
    );
}

void checkboxMenuItemRender(struct MenuBuilderElement* element, 
    int selection, 
    int materialIndex, 
    struct PrerenderedTextBatch* textBatch, 
    struct RenderState* renderState) {
    struct MenuCheckbox* checkbox = (struct MenuCheckbox*)element->data;

    if (materialIndex == SOLID_ENV_INDEX) {
        gSPDisplayList(renderState->dl++, checkbox->outline);
        renderState->dl = menuCheckboxRender(checkbox, renderState->dl);
    } else if (textBatch) {
        prerenderedBatchAdd(textBatch, checkbox->prerenderedText, selection == element->selectionIndex ? &gColorBlack : &gColorWhite);

        if (selection == element->selectionIndex) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
            gDPFillRectangle(
                renderState->dl++, 
                element->params->x + CHECKBOX_SIZE + 4, 
                element->params->y, 
                element->params->x + MENU_WIDTH - CHECKBOX_SIZE - 4,
                element->params->y + CHECKBOX_SIZE
            );
        }
    }
}

void sliderMenuItemInit(struct MenuBuilderElement* element) {
    struct MenuSlider* slider = malloc(sizeof(struct MenuSlider));

    short steps = element->params->params.slider.numberOfTicks;

    if (steps == 0) {
        steps = 9;
    }

    *slider = menuBuildSlider(
        element->params->x, 
        element->params->y, 
        element->params->params.slider.width,
        steps
    );
    element->data = slider;
}

#define FULL_SCROLL_TIME    2.0f
#define SCROLL_MULTIPLIER   (1.0f * FIXED_DELTA_TIME / (80 * FULL_SCROLL_TIME))

#define NEXT_TICK_TOLERANCE 0.001f

enum InputCapture sliderMenuItemUpdate(struct MenuBuilderElement* element, MenuActionCalback actionCallback, void* data) {
    struct MenuSlider* slider = (struct MenuSlider*)element->data;
    int controllerDir = controllerGetDirectionDown(0);
    
    if (element->params->params.slider.discrete) {
        int numTicks = element->params->params.slider.numberOfTicks;
        int currentValue = (int)floorf(slider->value * (numTicks - 1));
        int newValue = currentValue;

        if ((controllerDir & ControllerDirectionRight) || controllerGetButtonDown(0, A_BUTTON)) {
            ++newValue;
        } else if (controllerDir & ControllerDirectionLeft) {
            --newValue;
        }

        if (newValue < 0) {
            newValue = 0;
        } else if (newValue > numTicks - 1) {
            if (controllerGetButtonDown(0, A_BUTTON)) {
                newValue = 0;
            } else {
                newValue = numTicks - 1;
            }
        }

        if (newValue != currentValue) {
            struct MenuAction action;
            action.type = MenuElementTypeSlider;
            action.state.iSlider.value = newValue;
            actionCallback(data, element->selectionIndex, &action);
            slider->value = newValue;
        }

        if (numTicks > 1) {
            slider->value = (float)newValue / (float)(numTicks - 1);
        } else {
            slider->value = 0.0f;
        }
    } else {
        OSContPad* pad = controllersGetControllerData(0);
        float newValue = slider->value + pad->stick_x * SCROLL_MULTIPLIER;

        if (newValue > 1.0f) {
            newValue = 1.0f;
        } else if (newValue < 0.0f) {
            newValue = 0.0f;
        }

        int numTicks = element->params->params.slider.numberOfTicks;

        if (controllerGetButtonDown(0, R_JPAD | A_BUTTON)) {
            int currentValue = (int)floorf(newValue * (numTicks - 1) + NEXT_TICK_TOLERANCE);

            currentValue = currentValue + 1;

            if (currentValue > numTicks - 1) {
                if (controllerGetButtonDown(0, A_BUTTON)) {
                    currentValue = 0;
                } else {
                    currentValue = numTicks - 1;
                }
            }

            newValue = (float)currentValue / (float)(numTicks - 1);
        } else if (controllerGetButtonDown(0, L_JPAD)) {
            int currentValue = (int)ceilf(newValue * (numTicks - 1) - NEXT_TICK_TOLERANCE);

            currentValue = currentValue - 1;

            if (currentValue < 0) {
                currentValue = 0;
            }

            newValue = (float)currentValue / (float)(numTicks - 1);
        }

        if (newValue != slider->value) {
            struct MenuAction action;
            action.type = MenuElementTypeSlider;
            action.state.fSlider.value = newValue;
            actionCallback(data, element->selectionIndex, &action);
            slider->value = newValue;
        }
    }
    
    if (controllerGetButtonDown(0, L_JPAD | R_JPAD | A_BUTTON) || (element->params->params.slider.discrete && ((controllerDir & ControllerDirectionLeft) || (controllerDir & ControllerDirectionRight))))
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll); 

    return InputCapturePass;
}

void sliderMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    struct MenuSlider* slider = (struct MenuSlider*)element->data;
    gSPDisplayList(renderState->dl++, slider->back);
    renderState->dl = menuSliderRender(slider, renderState->dl);
}

struct MenuBuilderCallbacks gMenuTypeCallbacks[] = {
    {textMenuItemInit, NULL, textMenuItemRebuildText, textMenuItemRender},
    {checkboxMenuItemInit, checkboxMenuItemUpdate, checkboxMenuItemRebuildText, checkboxMenuItemRender},
    {sliderMenuItemInit, sliderMenuItemUpdate, NULL, sliderMenuItemRender},
};

void menuBuilderInit(
    struct MenuBuilder* menuBuilder, 
    struct MenuElementParams* params, 
    int elementCount, 
    int maxSelection, 
    MenuActionCalback actionCallback, 
    void* data
) {
    menuBuilder->elements = malloc(sizeof(struct MenuBuilderElement) * elementCount);
    menuBuilder->elementCount = elementCount;
    menuBuilder->selection = 0;
    menuBuilder->maxSelection = maxSelection;
    menuBuilder->actionCallback = actionCallback;
    menuBuilder->data = data;

    for (int i = 0; i < elementCount; ++i) {
        menuBuilder->elements[i].data = NULL;
        menuBuilder->elements[i].selectionIndex = params[i].selectionIndex;
        menuBuilder->elements[i].callbacks = &gMenuTypeCallbacks[params[i].type];
        menuBuilder->elements[i].params = &params[i];
        menuBuilder->elements[i].callbacks->init(&menuBuilder->elements[i]);
    }
}

enum InputCapture menuBuilderUpdate(struct MenuBuilder* menuBuilder) {
    if (controllerGetButtonDown(0, B_BUTTON)) {
        return InputCaptureExit;
    }

    int controllerDir = controllerGetDirectionDown(0);

    if (controllerDir & ControllerDirectionDown) {
        ++menuBuilder->selection;

        if (menuBuilder->selection == menuBuilder->maxSelection) {
            menuBuilder->selection = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        if (menuBuilder->selection == 0) {
            menuBuilder->selection = menuBuilder->maxSelection - 1;
        } else {
            --menuBuilder->selection;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        struct MenuBuilderElement* element = &menuBuilder->elements[i];

        if (element->callbacks->update && element->selectionIndex == menuBuilder->selection) {
            enum InputCapture direction = element->callbacks->update(element, menuBuilder->actionCallback, menuBuilder->data);

            if (direction != InputCapturePass) {
                return direction;
            }
        }
    }
    
    return InputCapturePass;
}

void menuBuilderRebuildText(struct MenuBuilder* menuBuilder) {
    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        if (menuBuilder->elements[i].callbacks->rebuildText) {
            menuBuilder->elements[i].callbacks->rebuildText(&menuBuilder->elements[i]);
        }
    }
}

void menuBuilderRender(struct MenuBuilder* menuBuilder, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        menuBuilder->elements[i].callbacks->render(&menuBuilder->elements[i], menuBuilder->selection, SOLID_ENV_INDEX, NULL, renderState);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);


    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        menuBuilder->elements[i].callbacks->render(&menuBuilder->elements[i], menuBuilder->selection, -1, batch, renderState);
    }
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);
}

void menuBuilderSetCheckbox(struct MenuBuilderElement* element, int value) {
    if (element->params->type != MenuElementTypeCheckbox) {
        return;
    }

    struct MenuCheckbox* checkbox = (struct MenuCheckbox*)element->data;
    checkbox->checked = value;
}

void menuBuilderSetFSlider(struct MenuBuilderElement* element, float value) {
    if (element->params->type != MenuElementTypeSlider || element->params->params.slider.discrete) {
        return;
    }

    struct MenuSlider* slider = (struct MenuSlider*)element->data;
    slider->value = value;
}

void menuBuilderSetISlider(struct MenuBuilderElement* element, int value) {
    if (element->params->type != MenuElementTypeSlider || 
        !element->params->params.slider.discrete || 
        element->params->params.slider.numberOfTicks <= 1) {
        return;
    }

    struct MenuSlider* slider = (struct MenuSlider*)element->data;
    slider->value = (float)value / (float)(element->params->params.slider.numberOfTicks - 1);
}
