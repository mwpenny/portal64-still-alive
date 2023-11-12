#ifndef __OPTIONS_VIDEO_OPTIONS_H__
#define __OPTIONS_VIDEO_OPTIONS_H__

#include "./menu.h"
#include "./menu_builder.h"
#include "../graphics/graphics.h"

enum VideoOption {
    VideoOptionWidescreen,
    VideoOptionInterlaced,
    VideoOptionSubtitles,
    VideoOptionCaptions,
    VideoOptionTextLanguage,
    VideoOptionCount,
};

struct VideoOptions {
    struct MenuBuilder menuBuilder;
};

void videoOptionsInit(struct VideoOptions* videoOptions);
void videoOptionsRebuildtext(struct VideoOptions* videoOptions);
enum InputCapture videoOptionsUpdate(struct VideoOptions* videoOptions);
void videoOptionsRender(struct VideoOptions* videoOptions, struct RenderState* renderState, struct GraphicsTask* task);


#endif