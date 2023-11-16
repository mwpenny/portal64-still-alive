#include "video_options.h"

#include "../font/dejavusans.h"
#include "./translations.h"
#include "../savefile/savefile.h"
#include "../main.h"

#include "../build/src/audio/subtitles.h"

#define MENU_Y      54
#define MENU_WIDTH  252
#define MENU_HEIGHT 124
#define MENU_X      ((SCREEN_WD - MENU_WIDTH) / 2)

struct MenuElementParams gVideoMenuParams[] = {
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 8,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_ASPECTWIDE,
            },
        },
        .selectionIndex = VideoOptionWidescreen,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 28,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_INTERLACED,
            },
        },
        .selectionIndex = VideoOptionInterlaced,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8, 
        .y = MENU_Y + 48,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_CAPTIONING,
            },
        },
        .selectionIndex = -1,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 68,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_SUBTITLES,
            },
        },
        .selectionIndex = VideoOptionSubtitles,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = MENU_X + 8, 
        .y = MENU_Y + 88,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_SUBTITLESANDSOUNDEFFECTS,
            }
        },
        .selectionIndex = VideoOptionCaptions,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8, 
        .y = MENU_Y + 124,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = NUM_SUBTITLE_LANGUAGES,
                .discrete = 1,
            },
        },
        .selectionIndex = VideoOptionTextLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8, 
        .y = MENU_Y + 108,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = AUDIO_TEXT_LANGUAGE,
            },
        },
        .selectionIndex = VideoOptionTextLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 125, 
        .y = MENU_Y + 108,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .message = "",
            },
        },
        .selectionIndex = VideoOptionTextLanguage,
    },
};

#define WIDESCREEN_INDEX 0
#define INTERLACED_INDEX 1
#define SUBTITLES_INDEX 3
#define CAPTIONS_INDEX 4
#define LANGUAGE_SLIDER_INDEX 5
#define LANGUAGE_TEXT_INDEX 7

char gIsInterlacedEnabled = 1;

void videoOptionsAction(void* data, int selection, struct MenuAction* action) {
    struct VideoOptions* videoOptions = (struct VideoOptions*)data;

    switch (selection) {
        case VideoOptionWidescreen:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveWideScreen;
            } else {
                gSaveData.controls.flags &= ~ControlSaveWideScreen;
            }
            break;
        case VideoOptionInterlaced:
            gIsInterlacedEnabled = action->state.checkbox.isChecked;
            setViMode(action->state.checkbox.isChecked);
            break;
        case VideoOptionSubtitles:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveSubtitlesEnabled;
                gSaveData.controls.flags &= ~ControlSaveAllSubtitlesEnabled;
                menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[CAPTIONS_INDEX], 0);
            } else {
                gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
            }
            break;
        case VideoOptionCaptions:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveAllSubtitlesEnabled;
                gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
                menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[SUBTITLES_INDEX], 0);
            } else {
                gSaveData.controls.flags &= ~ControlSaveAllSubtitlesEnabled;
            }
            break;
        case VideoOptionTextLanguage:
            gSaveData.controls.subtitleLanguage = action->state.iSlider.value;
            gVideoMenuParams[LANGUAGE_TEXT_INDEX].params.text.message = SubtitleLanguages[gSaveData.controls.subtitleLanguage];
            translationsReload(gSaveData.controls.subtitleLanguage);
            break;
    }
}

void videoOptionsInit(struct VideoOptions* videoOptions) {
    if (gSaveData.controls.subtitleLanguage < 0 || gSaveData.controls.subtitleLanguage >= NUM_SUBTITLE_LANGUAGES) {
        gSaveData.controls.subtitleLanguage = 0;
    }

    gVideoMenuParams[LANGUAGE_TEXT_INDEX].params.text.message = SubtitleLanguages[gSaveData.controls.subtitleLanguage];

    menuBuilderInit(
        &videoOptions->menuBuilder,
        gVideoMenuParams,
        sizeof(gVideoMenuParams) / sizeof(*gVideoMenuParams),
        VideoOptionCount,
        videoOptionsAction,
        videoOptions
    );

    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[WIDESCREEN_INDEX], (gSaveData.controls.flags & ControlSaveWideScreen) != 0);
    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[INTERLACED_INDEX], gIsInterlacedEnabled);

    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[CAPTIONS_INDEX], (gSaveData.controls.flags & ControlSaveAllSubtitlesEnabled) != 0);
    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[SUBTITLES_INDEX], (gSaveData.controls.flags & ControlSaveSubtitlesEnabled) != 0);

    menuBuilderSetISlider(&videoOptions->menuBuilder.elements[LANGUAGE_SLIDER_INDEX], gSaveData.controls.subtitleLanguage);
}

void videoOptionsRebuildtext(struct VideoOptions* videoOptions) {
    menuBuilderRebuildText(&videoOptions->menuBuilder);
}

enum InputCapture videoOptionsUpdate(struct VideoOptions* videoOptions) {
    return menuBuilderUpdate(&videoOptions->menuBuilder);
}

void videoOptionsRender(struct VideoOptions* videoOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    menuBuilderRender(&videoOptions->menuBuilder, renderState);
}

