#ifndef __MENU_AUDIO_OPTIONS_H__
#define __MENU_AUDIO_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

struct AudioOptions {
    short selectedItem;
};

void audioOptionsInit(struct AudioOptions* audioOptions);
enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions);
void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif