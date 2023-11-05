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

#define SCROLL_TICKS_VOLUME        9
#define SCROLL_INTERVALS_VOLUME    (int)maxf((SCROLL_TICKS_VOLUME-1), 1)
#define SCROLL_CHUNK_SIZE_VOLUME   (0x10000 / SCROLL_INTERVALS_VOLUME)

#define SCROLL_TICKS_SUBTITLES        (int)maxf(NUM_SUBTITLE_LANGUAGES, 1)
#define SCROLL_INTERVALS_SUBTITLES    (int)maxf((SCROLL_TICKS_SUBTITLES - 1), 1)
#define SCROLL_CHUNK_SIZE_SUBTITLES   (0x10000 / SCROLL_INTERVALS_SUBTITLES)

#define SCROLL_TICKS_LANGUAGE        (int)maxf(NUM_AUDIO_LANGUAGES, 1)
#define SCROLL_INTERVALS_LANGUAGE    (int)maxf((SCROLL_TICKS_LANGUAGE - 1), 1)
#define SCROLL_CHUNK_SIZE_LANGUAGE   (0x10000 / SCROLL_INTERVALS_LANGUAGE)

#define FULL_SCROLL_TIME    2.0f
#define SCROLL_MULTIPLIER   (int)(0x10000 * FIXED_DELTA_TIME / (80 * FULL_SCROLL_TIME))

void audioOptionsHandleSlider(short selectedItem, unsigned short* settingValue, float* sliderValue) {

    unsigned int chunk_size = 0;
    
    switch (selectedItem) {
        default:
        case AudioOptionGameVolume:
        case AudioOptionMusicVolume:
            chunk_size = SCROLL_CHUNK_SIZE_VOLUME;
            break;
        case AudioOptionSubtitlesLanguage:
            chunk_size = SCROLL_CHUNK_SIZE_SUBTITLES;
            break;
        case AudioOptionAudioLanguage:
            chunk_size = SCROLL_CHUNK_SIZE_LANGUAGE;
            break;
    }
    
    OSContPad* pad = controllersGetControllerData(0);

    int newValue = (int)*settingValue + pad->stick_x * SCROLL_MULTIPLIER;

    if (controllerGetButtonDown(0, A_BUTTON | R_JPAD)) {
        if (newValue >= 0xFFFF && controllerGetButtonDown(0, A_BUTTON)) {
            newValue = 0;
        } else {
            newValue = newValue + chunk_size;
            if ((0x10000 - newValue) > 0 && (0x10000 - newValue) < chunk_size && controllerGetButtonDown(0, A_BUTTON))
                newValue = 0x10000;
            else
                newValue = newValue - (newValue % chunk_size);
        }
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerGetButtonDown(0, L_JPAD)) {
        newValue = newValue - 1;
        newValue = newValue - (newValue % chunk_size);
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (newValue < 0) {
        newValue = 0;
    }

    if (newValue > 0xFFFF) {
        newValue = 0xFFFF;
    }

    *settingValue = newValue;
    *sliderValue = (float)newValue / 0xFFFF;
}

void audioOptionsRenderText(struct AudioOptions* audioOptions) {
    audioOptions->subtitlesEnabled.prerenderedText = menuBuildPrerenderedText(
        &gDejaVuSansFont, 
        translationsGet(GAMEUI_SUBTITLESANDSOUNDEFFECTS), 
        audioOptions->subtitlesEnabled.x + CHECKBOX_SIZE + 6, 
        audioOptions->subtitlesEnabled.y
    );
    audioOptions->allSubtitlesEnabled.prerenderedText = menuBuildPrerenderedText(
        &gDejaVuSansFont, 
        "All Captions", 
        audioOptions->allSubtitlesEnabled.x + CHECKBOX_SIZE + 6, 
        audioOptions->allSubtitlesEnabled.y
    );
    audioOptions->subtitlesLanguageText = menuBuildPrerenderedText(&gDejaVuSansFont, "Captions Language: ", GAMEPLAY_X + 8, GAMEPLAY_Y + 88);
    audioOptions->subtitlesLanguageDynamicText = menuBuildPrerenderedText(&gDejaVuSansFont, SubtitleLanguages[gSaveData.controls.subtitleLanguage], GAMEPLAY_X + 125, GAMEPLAY_Y + 88);
    audioOptions->audioLanguageText = menuBuildPrerenderedText(&gDejaVuSansFont, "Audio Language: ", GAMEPLAY_X + 8, GAMEPLAY_Y + 124);
    audioOptions->audioLanguageDynamicText = menuBuildPrerenderedText(&gDejaVuSansFont, AudioLanguages[gSaveData.audio.audioLanguage], GAMEPLAY_X + 125, GAMEPLAY_Y + 124);
}

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
};

void audioOptionsActoin(void* data, int selection, struct MenuAction* action) {
    switch (selection) {
        case AudioOptionGameVolume:
            gSaveData.audio.soundVolume = (int)(0xFFFF * action->state.fSlider.value);
            soundPlayerGameVolumeUpdate();
            break;
        case AudioOptionMusicVolume:
            gSaveData.audio.musicVolume = (int)(0xFFFF * action->state.fSlider.value);
            soundPlayerGameVolumeUpdate();
            break;
    }
}

void audioOptionsInit(struct AudioOptions* audioOptions) {
    menuBuilderInit(
        &audioOptions->menuBuilder,
        gAudioMenuParams,
        sizeof(gAudioMenuParams) / sizeof(*gAudioMenuParams),
        AudioOptionCount,
        audioOptionsActoin,
        audioOptions
    );

    audioOptions->selectedItem = AudioOptionGameVolume;
    int temp;

    audioOptions->subtitlesEnabled = menuBuildCheckbox(&gDejaVuSansFont, translationsGet(GAMEUI_SUBTITLESANDSOUNDEFFECTS), GAMEPLAY_X + 8, GAMEPLAY_Y + 48, 1);
    audioOptions->subtitlesEnabled.checked = (gSaveData.controls.flags & ControlSaveSubtitlesEnabled) != 0;

    audioOptions->allSubtitlesEnabled = menuBuildCheckbox(&gDejaVuSansFont, "All Captions", GAMEPLAY_X + 8, GAMEPLAY_Y + 68, 1);
    audioOptions->allSubtitlesEnabled.checked = (gSaveData.controls.flags & ControlSaveAllSubtitlesEnabled) != 0;

    audioOptions->subtitlesLanguage= menuBuildSlider(GAMEPLAY_X + 8, GAMEPLAY_Y + 104, 232, NUM_SUBTITLE_LANGUAGES);
    temp = (int)(maxf(NUM_SUBTITLE_LANGUAGES-1, 1));
    audioOptions->subtitles_language_temp = (0xFFFF/temp)* gSaveData.controls.subtitleLanguage;
    if ((0xFFFF - audioOptions->subtitles_language_temp) > 0 && (0xFFFF - audioOptions->subtitles_language_temp) < (0xFFFF/temp))
      audioOptions->subtitles_language_temp = 0xFFFF;
    audioOptions->subtitlesLanguage.value = (float)(audioOptions->subtitles_language_temp)/0xFFFF;


    audioOptions->audioLanguage= menuBuildSlider(GAMEPLAY_X + 8, GAMEPLAY_Y + 140, 232, NUM_AUDIO_LANGUAGES);
    temp = (int)(maxf(NUM_AUDIO_LANGUAGES-1, 1));
    audioOptions->audio_language_temp = (int)((0xFFFF/temp) * gSaveData.audio.audioLanguage);
    if ((0xFFFF - audioOptions->audio_language_temp) > 0 && (0xFFFF - audioOptions->audio_language_temp) < (0xFFFF/temp))
      audioOptions->audio_language_temp = 0xFFFF;
    audioOptions->audioLanguage.value = (float)(audioOptions->audio_language_temp)/0xFFFF;

    audioOptionsRenderText(audioOptions);
}

void audioOptionsRebuildtext(struct AudioOptions* audioOptions) {
    prerenderedTextFree(audioOptions->subtitlesEnabled.prerenderedText);
    prerenderedTextFree(audioOptions->allSubtitlesEnabled.prerenderedText);
    prerenderedTextFree(audioOptions->subtitlesLanguageText);
    prerenderedTextFree(audioOptions->subtitlesLanguageDynamicText);
    prerenderedTextFree(audioOptions->audioLanguageText);
    prerenderedTextFree(audioOptions->audioLanguageDynamicText);

    audioOptionsRenderText(audioOptions);

    menuBuilderRebuildText(&audioOptions->menuBuilder);
}

enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
    }

    if (controllerDir & ControllerDirectionDown) {
        ++audioOptions->selectedItem;

        if (audioOptions->selectedItem == AudioOptionCount) {
            audioOptions->selectedItem = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        if (audioOptions->selectedItem == 0) {
            audioOptions->selectedItem = AudioOptionCount - 1;
        } else {
            --audioOptions->selectedItem;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);
    }

    switch (audioOptions->selectedItem) {
        case AudioOptionSubtitlesEnabled:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                audioOptions->subtitlesEnabled.checked = !audioOptions->subtitlesEnabled.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

                if (audioOptions->subtitlesEnabled.checked) {
                    gSaveData.controls.flags |= ControlSaveSubtitlesEnabled;
                    gSaveData.controls.flags &= ~ControlSaveAllSubtitlesEnabled;
                    audioOptions->allSubtitlesEnabled.checked = 0;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
                }
            }
            break;
        case AudioOptionAllSubtitlesEnabled:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                audioOptions->allSubtitlesEnabled.checked = !audioOptions->allSubtitlesEnabled.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL, SoundTypeAll);

                if (audioOptions->allSubtitlesEnabled.checked) {
                    gSaveData.controls.flags |= ControlSaveAllSubtitlesEnabled;
                    gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
                    audioOptions->subtitlesEnabled.checked = 0;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveAllSubtitlesEnabled;
                }
            }
            break;
        case AudioOptionSubtitlesLanguage:
            audioOptionsHandleSlider(audioOptions->selectedItem, &audioOptions->subtitles_language_temp, &audioOptions->subtitlesLanguage.value);
            int temp = (int)((audioOptions->subtitles_language_temp * (1.0f/0xFFFF) * NUM_SUBTITLE_LANGUAGES));
            temp = (int)minf(maxf(0.0, temp), NUM_SUBTITLE_LANGUAGES-1);
            gSaveData.controls.subtitleLanguage = temp;
            translationsReload(temp);
            break;
        case AudioOptionAudioLanguage:
            audioOptionsHandleSlider(audioOptions->selectedItem, &audioOptions->audio_language_temp, &audioOptions->audioLanguage.value);
            int tempAudio = (int)((audioOptions->audio_language_temp * (1.0f/0xFFFF) * NUM_AUDIO_LANGUAGES));
            tempAudio = (int)minf(maxf(0.0f, tempAudio), NUM_AUDIO_LANGUAGES-1);
            gSaveData.audio.audioLanguage = tempAudio;
            audioOptions->audioLanguageDynamicText = menuBuildPrerenderedText(&gDejaVuSansFont, AudioLanguages[gSaveData.audio.audioLanguage], GAMEPLAY_X + 125, GAMEPLAY_Y + 124);
            break;
            
    }
    

    if (audioOptions->selectedItem == AudioOptionSubtitlesLanguage || 
        audioOptions->selectedItem == AudioOptionGameVolume ||
        audioOptions->selectedItem == AudioOptionMusicVolume ||
        audioOptions->selectedItem == AudioOptionAudioLanguage){

        if ((controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG))) {
            return MenuDirectionLeft;
        }
        if ((controllerGetButtonDown(0, R_TRIG))) {
            return MenuDirectionRight;
        }
    }
    else{
        if (controllerDir & ControllerDirectionLeft || controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG)) {
            return MenuDirectionLeft;
        }
        if (controllerDir & ControllerDirectionRight || controllerGetButtonDown(0, R_TRIG)) {
            return MenuDirectionRight;
        }
    }

    return MenuDirectionStay;
}

void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, audioOptions->subtitlesEnabled.outline);
    renderState->dl = menuCheckboxRender(&audioOptions->subtitlesEnabled, renderState->dl);

    gSPDisplayList(renderState->dl++, audioOptions->allSubtitlesEnabled.outline);
    renderState->dl = menuCheckboxRender(&audioOptions->allSubtitlesEnabled, renderState->dl);

    gSPDisplayList(renderState->dl++, audioOptions->subtitlesLanguage.back);
    renderState->dl = menuSliderRender(&audioOptions->subtitlesLanguage, renderState->dl);

    gSPDisplayList(renderState->dl++, audioOptions->audioLanguage.back);
    renderState->dl = menuSliderRender(&audioOptions->audioLanguage, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    prerenderedBatchAdd(batch, audioOptions->subtitlesEnabled.prerenderedText, audioOptions->selectedItem == AudioOptionSubtitlesEnabled ? &gSelectionGray : &gColorWhite);
    prerenderedBatchAdd(batch, audioOptions->allSubtitlesEnabled.prerenderedText, audioOptions->selectedItem == AudioOptionAllSubtitlesEnabled ? &gSelectionGray : &gColorWhite);
    prerenderedBatchAdd(batch, audioOptions->subtitlesLanguageText, audioOptions->selectedItem == AudioOptionSubtitlesLanguage ? &gSelectionGray : &gColorWhite);
    prerenderedBatchAdd(batch, audioOptions->subtitlesLanguageDynamicText, audioOptions->selectedItem == AudioOptionSubtitlesLanguage ? &gSelectionGray : &gColorWhite);
    prerenderedBatchAdd(batch, audioOptions->audioLanguageText, audioOptions->selectedItem == AudioOptionAudioLanguage ? &gSelectionGray : &gColorWhite);
    prerenderedBatchAdd(batch, audioOptions->audioLanguageDynamicText, audioOptions->selectedItem == AudioOptionAudioLanguage ? &gSelectionGray : &gColorWhite);

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    menuBuilderRender(&audioOptions->menuBuilder, renderState);
}
