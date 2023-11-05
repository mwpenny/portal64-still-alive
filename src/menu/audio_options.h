#ifndef __MENU_AUDIO_OPTIONS_H__
#define __MENU_AUDIO_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"
#include "menu_builder.h"

enum AudioOption {
    AudioOptionGameVolume,
    AudioOptionMusicVolume,
    AudioOptionSubtitlesEnabled,
    AudioOptionAllSubtitlesEnabled,
    AudioOptionSubtitlesLanguage,
    AudioOptionAudioLanguage,
    AudioOptionCount,
};

struct AudioOptions {
    struct MenuBuilder menuBuilder;
};

void audioOptionsInit(struct AudioOptions* audioOptions);
void audioOptionsRebuildtext(struct AudioOptions* audioOptions);
enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions);
void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif
