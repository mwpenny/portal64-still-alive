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

    if (controllerDir & ControllerDirectionLeft) {
        return MenuDirectionLeft;
    }

    if (controllerDir & ControllerDirectionRight) {
        return MenuDirectionRight;
    }

    return MenuDirectionStay;
}

void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task) {

}