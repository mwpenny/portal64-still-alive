#ifndef __MENU_AUDIO_OPTIONS_H__
#define __MENU_AUDIO_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum AudioOption {
    AudioOptionSubtitlesEnabled,

    AudioOptionCount,
};

struct AudioOptions {
    struct MenuCheckbox subtitlesEnabled;
    short selectedItem;
};

void audioOptionsInit(struct AudioOptions* audioOptions);
enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions);
void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif