#include "audio_options.h"

#include "../controls/controller.h"
#include "../savefile/savefile.h"
#include "../font/dejavusans.h"
#include "../audio/soundplayer.h"
#include "../build/assets/materials/ui.h"
#include "../build/src/audio/clips.h"

#define GAMEPLAY_Y      54
#define GAMEPLAY_WIDTH  252
#define GAMEPLAY_HEIGHT 124
#define GAMEPLAY_X      ((SCREEN_WD - GAMEPLAY_WIDTH) / 2)

void audioOptionsInit(struct AudioOptions* audioOptions) {
    audioOptions->selectedItem = AudioOptionSubtitlesEnabled;

    audioOptions->subtitlesEnabled = menuBuildCheckbox(&gDejaVuSansFont, "Closed Captions", GAMEPLAY_X + 8, GAMEPLAY_Y + 8);
    audioOptions->subtitlesEnabled.checked = (gSaveData.controls.flags & ControlSaveSubtitlesEnabled) != 0;
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
    }

    if (controllerDir & ControllerDirectionUp) {
        if (audioOptions->selectedItem == 0) {
            audioOptions->selectedItem = AudioOptionCount - 1;
        } else {
            --audioOptions->selectedItem;
        }
    }

    switch (audioOptions->selectedItem) {
        case AudioOptionSubtitlesEnabled:
            if (controllerGetButtonDown(0, A_BUTTON)) {
                audioOptions->subtitlesEnabled.checked = !audioOptions->subtitlesEnabled.checked;
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 0.5f, NULL, NULL);

                if (audioOptions->subtitlesEnabled.checked) {
                    gSaveData.controls.flags |= ControlSaveSubtitlesEnabled;
                } else {
                    gSaveData.controls.flags &= ~ControlSaveSubtitlesEnabled;
                }
            }

            break;
    }

    if ((controllerDir & ControllerDirectionLeft || controllerGetButtonDown(0, L_TRIG) || controllerGetButtonDown(0, Z_TRIG))) {
        return MenuDirectionLeft;
    }

    if ((controllerDir & ControllerDirectionRight || controllerGetButtonDown(0, R_TRIG))) {
        return MenuDirectionRight;
    }

    return MenuDirectionStay;
}

void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    
    gSPDisplayList(renderState->dl++, audioOptions->subtitlesEnabled.outline);
    renderState->dl = menuCheckboxRender(&audioOptions->subtitlesEnabled, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_INDEX]);

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, audioOptions->selectedItem == AudioOptionSubtitlesEnabled, &gSelectionGray, &gColorWhite);
    gSPDisplayList(renderState->dl++, audioOptions->subtitlesEnabled.text);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_INDEX]);
}