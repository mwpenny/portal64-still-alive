#ifndef __MENU_AUDIO_OPTIONS_H__
#define __MENU_AUDIO_OPTIONS_H__

#include "./menu.h"
#include "../graphics/graphics.h"

enum AudioOption {
    AudioOptionGameVolume,
    AudioOptionMusicVolume,
    AudioOptionSubtitlesEnabled,
    AudioOptionAllSubtitlesEnabled,
    AudioOptionSubtitlesLanguage,

    AudioOptionCount,
};

struct AudioOptions {
    struct MenuSlider gameVolume;
    struct MenuSlider musicVolume;
    struct MenuCheckbox subtitlesEnabled;
    struct MenuCheckbox allSubtitlesEnabled;
    struct MenuSlider subtitlesLanguage;
    Gfx* gameVolumeText;
    Gfx* musicVolumeText;
    Gfx* subtitlesLanguageText;
    Gfx* subtitlesLanguageDynamicText;
    unsigned short subtitles_language_temp;
    short selectedItem;
};

void audioOptionsInit(struct AudioOptions* audioOptions);
enum MenuDirection audioOptionsUpdate(struct AudioOptions* audioOptions);
void audioOptionsRender(struct AudioOptions* audioOptions, struct RenderState* renderState, struct GraphicsTask* task);

#endif