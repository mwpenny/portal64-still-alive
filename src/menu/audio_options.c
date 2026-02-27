#include "audio_options.h"

#include "audio/soundplayer.h"
#include "font/dejavu_sans.h"
#include "savefile/savefile.h"
#include "system/controller.h"
#include "system/screen.h"

#include "codegen/assets/audio/languages.h"
#include "codegen/assets/strings/strings.h"

#define MENU_Y      54
#define MENU_WIDTH  252
#define MENU_HEIGHT 124
#define MENU_X      ((SCREEN_WD - MENU_WIDTH) / 2)

struct MenuElementParams gAudioMenuParams[] = {
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8,
        .y = MENU_Y + 8,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_SOUNDEFFECTVOLUME,
            },
        },
        .selectionIndex = AudioOptionGameVolume,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8,
        .y = MENU_Y + 24,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = AudioOptionGameVolume,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8,
        .y = MENU_Y + 44,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_MUSICVOLUME,
            },
        },
        .selectionIndex = AudioOptionMusicVolume,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8,
        .y = MENU_Y + 60,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = AudioOptionMusicVolume,
    },
    {
        .type = MenuElementTypeSlider,
        .x = MENU_X + 8,
        .y = MENU_Y + 96,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = NUM_AUDIO_LANGUAGES,
                .discrete = 1,
            },
        },
        .selectionIndex = AudioOptionAudioLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8,
        .y = MENU_Y + 80,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = AUDIO_AUDIO_LANGUAGE,
            },
        },
        .selectionIndex = AudioOptionAudioLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = MENU_X + 8 + 232,
        .y = MENU_Y + 80,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .message = "",
                .rightAlign = 1,
            },
        },
        .selectionIndex = AudioOptionAudioLanguage,
    },
};

#define AUDIO_VOLUME_VALUE_INDEX   1
#define MUSIC_VOLUME_VALUE_INDEX   3

#define AUDIO_LANGUAGE_VALUE_INDEX 4
#define AUDIO_LANGUAGE_TEXT_INDEX 6

void audioOptionsAction(void* data, int selection, struct MenuAction* action) {
    struct AudioOptions* audioOptions = (struct AudioOptions*)data;

    switch (selection) {
        case AudioOptionGameVolume:
            gSaveData.audio.soundVolume = (int)(0xFFFF * action->state.fSlider.value + 0.5f);
            soundPlayerUpdateVolumeLevels();
            break;
        case AudioOptionMusicVolume:
            gSaveData.audio.musicVolume = (int)(0xFFFF * action->state.fSlider.value + 0.5f);
            soundPlayerUpdateVolumeLevels();
            break;
        case AudioOptionAudioLanguage:
            gSaveData.audio.audioLanguage = action->state.iSlider.value;

            struct MenuBuilderElement* element = &audioOptions->menuBuilder.elements[AUDIO_LANGUAGE_TEXT_INDEX];
            element->params->params.text.message = AudioLanguages[gSaveData.audio.audioLanguage];
            element->callbacks->rebuildText(element, 1 /* deferFree */);

            break;
    }
}

void audioOptionsInit(struct AudioOptions* audioOptions) {
    if (gSaveData.audio.audioLanguage < 0 || gSaveData.audio.audioLanguage >= NUM_AUDIO_LANGUAGES) {
        gSaveData.audio.audioLanguage = 0;
    }

    gAudioMenuParams[AUDIO_LANGUAGE_TEXT_INDEX].params.text.message = AudioLanguages[gSaveData.audio.audioLanguage];

    menuBuilderInit(
        &audioOptions->menuBuilder,
        gAudioMenuParams,
        sizeof(gAudioMenuParams) / sizeof(*gAudioMenuParams),
        AudioOptionCount,
        audioOptionsAction,
        audioOptions
    );

    menuBuilderSetFSlider(&audioOptions->menuBuilder.elements[AUDIO_VOLUME_VALUE_INDEX], gSaveData.audio.soundVolume / (float)(0xFFFF));
    menuBuilderSetFSlider(&audioOptions->menuBuilder.elements[MUSIC_VOLUME_VALUE_INDEX], gSaveData.audio.musicVolume / (float)(0xFFFF));
    menuBuilderSetISlider(&audioOptions->menuBuilder.elements[AUDIO_LANGUAGE_VALUE_INDEX], gSaveData.audio.audioLanguage);
}

void audioOptionsRebuildtext(struct AudioOptions* audioOptions) {
    menuBuilderRebuildText(&audioOptions->menuBuilder);
}

enum InputCapture audioOptionsUpdate(struct AudioOptions* audioOptions) {
    return menuBuilderUpdate(&audioOptions->menuBuilder);
}

void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    menuBuilderRender(&audioOptions->menuBuilder, renderState);
}
