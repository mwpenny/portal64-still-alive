#include "video_options.h"

#include "font/dejavu_sans.h"
#include "main.h"
#include "savefile/savefile.h"
#include "strings/translations.h"
#include "system/screen.h"

#include "codegen/assets/strings/strings.h"

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
                .numberOfTicks = NUM_STRING_LANGUAGES,
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
                gSaveData.video.flags |= VideoSaveFlagsWideScreen;
            } else {
                gSaveData.video.flags &= ~VideoSaveFlagsWideScreen;
            }
            break;
        case VideoOptionInterlaced:
            gIsInterlacedEnabled = action->state.checkbox.isChecked;
            setViMode(action->state.checkbox.isChecked);
            break;
        case VideoOptionSubtitles:
            if (action->state.checkbox.isChecked) {
                gSaveData.video.flags |= VideoSaveFlagsSubtitlesEnabled;
                gSaveData.video.flags &= ~VideoSaveFlagsCaptionsEnabled;
                menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[CAPTIONS_INDEX], 0);
            } else {
                gSaveData.video.flags &= ~VideoSaveFlagsSubtitlesEnabled;
            }
            break;
        case VideoOptionCaptions:
            if (action->state.checkbox.isChecked) {
                gSaveData.video.flags |= VideoSaveFlagsCaptionsEnabled;
                gSaveData.video.flags &= ~VideoSaveFlagsSubtitlesEnabled;
                menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[SUBTITLES_INDEX], 0);
            } else {
                gSaveData.video.flags &= ~VideoSaveFlagsCaptionsEnabled;
            }
            break;
        case VideoOptionTextLanguage:
            gSaveData.video.textLanguage = action->state.iSlider.value;
            gVideoMenuParams[LANGUAGE_TEXT_INDEX].params.text.message = StringLanguages[gSaveData.video.textLanguage];
            translationsReload(gSaveData.video.textLanguage);
            break;
    }
}

void videoOptionsInit(struct VideoOptions* videoOptions) {
    if (gSaveData.video.textLanguage < 0 || gSaveData.video.textLanguage >= NUM_STRING_LANGUAGES) {
        gSaveData.video.textLanguage = 0;
    }

    gVideoMenuParams[LANGUAGE_TEXT_INDEX].params.text.message = StringLanguages[gSaveData.video.textLanguage];

    menuBuilderInit(
        &videoOptions->menuBuilder,
        gVideoMenuParams,
        sizeof(gVideoMenuParams) / sizeof(*gVideoMenuParams),
        VideoOptionCount,
        videoOptionsAction,
        videoOptions
    );

    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[WIDESCREEN_INDEX], (gSaveData.video.flags & VideoSaveFlagsWideScreen) != 0);
    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[INTERLACED_INDEX], gIsInterlacedEnabled);

    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[CAPTIONS_INDEX], (gSaveData.video.flags & VideoSaveFlagsCaptionsEnabled) != 0);
    menuBuilderSetCheckbox(&videoOptions->menuBuilder.elements[SUBTITLES_INDEX], (gSaveData.video.flags & VideoSaveFlagsSubtitlesEnabled) != 0);

    menuBuilderSetISlider(&videoOptions->menuBuilder.elements[LANGUAGE_SLIDER_INDEX], gSaveData.video.textLanguage);
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

