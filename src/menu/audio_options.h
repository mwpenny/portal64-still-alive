#ifndef __MENU_AUDIO_OPTIONS_H__
#define __MENU_AUDIO_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum AudioOption {
    AudioOptionSubtitlesEnabled,
    AudioOptionSubtitlesLanguage,

    AudioOptionCount,
};

struct AudioOptions {
    struct MenuCheckbox subtitlesEnabled;
    struct MenuSlider subtitlesLanguage;
    Gfx* subtitlesLanguageText;
    Gfx* subtitlesLanguageDynamicText;
    unsigned short subtitles_language_temp;
    short selectedItem;
};

void audioOptionsInit(struct AudioOptions* audioOptions);
enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions);
void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif