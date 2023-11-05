#include "audio_options.h"

#include "../controls/controller.h"
#include "../savefile/savefile.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../build/src/audio/subtitles.h"
#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"
#include "../build/src/audio/languages.h"
#include "./translations.h"

#define GAMEPLAY_Y      54
#define GAMEPLAY_WIDTH  252
#define GAMEPLAY_HEIGHT 124
#define GAMEPLAY_X      ((SCREEN_WD - GAMEPLAY_WIDTH) / 2)

struct MenuElementParams gAudioMenuParams[] = {
    {
        .type = MenuElementTypeText,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 8,
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
        .x = GAMEPLAY_X + 120, 
        .y = GAMEPLAY_Y + 8,
        .params = {
            .slider = {
                .width = 120,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = AudioOptionGameVolume,
    },
    {
        .type = MenuElementTypeText,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 28,
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
        .x = GAMEPLAY_X + 120, 
        .y = GAMEPLAY_Y + 28,
        .params = {
            .slider = {
                .width = 120,
                .numberOfTicks = 9,
                .discrete = 0,
            },
        },
        .selectionIndex = AudioOptionMusicVolume,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 48,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = GAMEUI_SUBTITLESANDSOUNDEFFECTS,
            },
        },
        .selectionIndex = AudioOptionSubtitlesEnabled,
    },
    {
        .type = MenuElementTypeCheckbox,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 68,
        .params = {
            .checkbox = {
                .font = &gDejaVuSansFont,
                .messageId = AUDIO_ALL_CAPTIONS,
            }
        },
        .selectionIndex = AudioOptionAllSubtitlesEnabled,
    },
    {
        .type = MenuElementTypeSlider,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 104,
        .params = {
            .slider = {
                .width = 232,
                .numberOfTicks = NUM_SUBTITLE_LANGUAGES,
                .discrete = 1,
            },
        },
        .selectionIndex = AudioOptionSubtitlesLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 88,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = AUDIO_TEXT_LANGUAGE,
            },
        },
        .selectionIndex = AudioOptionSubtitlesLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = GAMEPLAY_X + 125, 
        .y = GAMEPLAY_Y + 88,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .message = "",
            },
        },
        .selectionIndex = AudioOptionSubtitlesLanguage,
    },
    {
        .type = MenuElementTypeSlider,
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 140,
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
        .x = GAMEPLAY_X + 8, 
        .y = GAMEPLAY_Y + 124,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .messageId = AUDIO_TEXT_LANGUAGE,
            },
        },
        .selectionIndex = AudioOptionAudioLanguage,
    },
    {
        .type = MenuElementTypeText,
        .x = GAMEPLAY_X + 125, 
        .y = GAMEPLAY_Y + 124,
        .params = {
            .text = {
                .font = &gDejaVuSansFont,
                .message = "",
            },
        },
        .selectionIndex = AudioOptionAudioLanguage,
    },
};

#define AUDIO_VOLUME_VALUE_INDEX   1
#define MUSIC_VOLUME_VALUE_INDEX   3

#define SUBTITLES_CHECKBOX_INDEX   4
#define ALL_SUBTITLES_CHECKBOX_INDEX   5

#define TEXT_LANGUAGE_VALUE_INDEX 6
#define TEXT_LANGUAGE_TEXT_INDEX 8
#define AUDIO_LANGUAGE_VALUE_INDEX 9
#define AUDIO_LANGUAGE_TEXT_INDEX 11

void audioOptionsActoin(void* data, int selection, struct MenuAction* action) {
    struct AudioOptions* audioOptions = (struct AudioOptions*)data;

    switch (selection) {
        case AudioOptionGameVolume:
            gSaveData.audio.soundVolume = (int)(0xFFFF * action->state.fSlider.value);
            soundPlayerGameVolumeUpdate();
            break;
        case AudioOptionMusicVolume:
            gSaveData.audio.musicVolume = (int)(0xFFFF * action->state.fSlider.value);
            soundPlayerGameVolumeUpdate();
            break;
        case AudioOptionSubtitlesEnabled:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveSubtitlesEnabled;
                gSaveData.controls.flags &= ~ControlSaveAllSubtitlesEnabled;
                struct MenuCheckbox* allSubtitles = (struct MenuCheckbox*)audioOptions->menuBuilder.elements[ALL_SUBTITLES_CHECKBOX_INDEX].data;
                allSubtitles->checked = 0;
            } else {
                gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
            }
            break;
        case AudioOptionAllSubtitlesEnabled:
            if (action->state.checkbox.isChecked) {
                gSaveData.controls.flags |= ControlSaveAllSubtitlesEnabled;
                gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
                struct MenuCheckbox* subtitles = (struct MenuCheckbox*)audioOptions->menuBuilder.elements[SUBTITLES_CHECKBOX_INDEX].data;
                subtitles->checked = 0;
            } else {
                gSaveData.controls.flags &= ~ControlSaveAllSubtitlesEnabled;
            }
            break;
        case AudioOptionSubtitlesLanguage:
            gSaveData.controls.subtitleLanguage = action->state.iSlider.value;
            gAudioMenuParams[TEXT_LANGUAGE_TEXT_INDEX].params.text.message = SubtitleLanguages[gSaveData.controls.subtitleLanguage];
            translationsReload(gSaveData.controls.subtitleLanguage);
            break;
        case AudioOptionAudioLanguage:
            gSaveData.audio.audioLanguage = action->state.iSlider.value;
            audioOptions->menuBuilder.elements[AUDIO_LANGUAGE_TEXT_INDEX].data = menuBuildPrerenderedText(&gDejaVuSansFont, AudioLanguages[gSaveData.audio.audioLanguage], GAMEPLAY_X + 125, GAMEPLAY_Y + 124);
            break;
    }
}

void audioOptionsInit(struct AudioOptions* audioOptions) {
    if (gSaveData.controls.subtitleLanguage < 0 || gSaveData.controls.subtitleLanguage >= NUM_SUBTITLE_LANGUAGES) {
        gSaveData.controls.subtitleLanguage = 0;
    }

    if (gSaveData.audio.audioLanguage < 0 || gSaveData.audio.audioLanguage >= NUM_AUDIO_LANGUAGES) {
        gSaveData.audio.audioLanguage = 0;
    }

    gAudioMenuParams[TEXT_LANGUAGE_TEXT_INDEX].params.text.message = SubtitleLanguages[gSaveData.controls.subtitleLanguage];
    gAudioMenuParams[AUDIO_LANGUAGE_TEXT_INDEX].params.text.message = AudioLanguages[gSaveData.audio.audioLanguage];

    menuBuilderInit(
        &audioOptions->menuBuilder,
        gAudioMenuParams,
        sizeof(gAudioMenuParams) / sizeof(*gAudioMenuParams),
        AudioOptionCount,
        audioOptionsActoin,
        audioOptions
    );

    menuBuilderSetFSlider(&audioOptions->menuBuilder.elements[AUDIO_VOLUME_VALUE_INDEX], gSaveData.audio.soundVolume / (float)(0xFFFF));
    menuBuilderSetFSlider(&audioOptions->menuBuilder.elements[MUSIC_VOLUME_VALUE_INDEX], gSaveData.audio.musicVolume / (float)(0xFFFF));

    menuBuilderSetCheckbox(&audioOptions->menuBuilder.elements[ALL_SUBTITLES_CHECKBOX_INDEX], (gSaveData.controls.flags & ControlSaveAllSubtitlesEnabled) != 0);
    menuBuilderSetCheckbox(&audioOptions->menuBuilder.elements[SUBTITLES_CHECKBOX_INDEX], (gSaveData.controls.flags & ControlSaveSubtitlesEnabled) != 0);

    menuBuilderSetISlider(&audioOptions->menuBuilder.elements[TEXT_LANGUAGE_VALUE_INDEX], gSaveData.controls.subtitleLanguage);
    menuBuilderSetISlider(&audioOptions->menuBuilder.elements[AUDIO_LANGUAGE_VALUE_INDEX], gSaveData.audio.audioLanguage);
}

void audioOptionsRebuildtext(struct AudioOptions* audioOptions) {
    menuBuilderRebuildText(&audioOptions->menuBuilder);
}

enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions) {
    return menuBuilderUpdate(&audioOptions->menuBuilder);
}

void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    menuBuilderRender(&audioOptions->menuBuilder, renderState);
}
