#ifndef _GRAPHICS_IMAGE_H
#define _GRAPHICS_IMAGE_H

#include "graphics/color.h"
#include "renderstate.h"

void graphicsCopyImage(
    struct RenderState* state,
    void* image,
    int imageWidth,
    int imageHeight,
    int srcX,
    int srcY,
    int srcWidth,
    int srcHeight,
    int screenX,
    int screenY,
    struct Coloru8 color
);

#endif
