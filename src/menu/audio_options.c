#include "audio_options.h"

#include "../controls/controller.h"

void audioOptionsInit(struct AudioOptions* audioOptions) {
    audioOptions->selectedItem = 0;
}

enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions) {
    int controllerDir = controllerGetDirectionDown(0);

    if (controllerGetButtonDown(0, B_BUTTON)) {
        return MenuDirectionUp;
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

}