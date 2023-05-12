#include "menu.h"

#include "../util/memory.h"

struct Coloru8 gSelectionOrange = {255, 156, 0, 255};
struct Coloru8 gSelectionGray = {201, 201, 201, 255};
struct Coloru8 gBorderHighlight = {193, 193, 193, 255};
struct Coloru8 gBorderDark = {86, 86, 86, 255};

Gfx* menuBuildText(struct Font* font, char* message, int x, int y) {
    Gfx* result = malloc(sizeof(Gfx) * (fontCountGfx(font, message) + 1));
    Gfx* dl = result;

    dl = fontRender(font, message, x, y, dl);
    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuBuildBorder(int x, int y, int width, int height) {
    Gfx* result = malloc(sizeof(Gfx) * 7 * 3 + 1);
    Gfx* dl = result;

    gSPTextureRectangle(
        dl++,
        x << 2, y << 2,
        (x + 4) << 2, (y + 4) << 2,
        G_TX_RENDERTILE,
        0 << 5, 0 << 5,
        0x400, 0x400
    );

    gSPTextureRectangle(
        dl++,
        (x + 4) << 2, y << 2,
        (x + width - 4) << 2, (y + 4) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0, 0
    );

    gSPTextureRectangle(
        dl++,
        (x + width - 4) << 2, y << 2,
        (x + width) << 2, (y + 4) << 2,
        G_TX_RENDERTILE,
        4 << 5, 0 << 5,
        0x400, 0x400
    );

    gSPTextureRectangle(
        dl++,
        x << 2, (y + 4) << 2,
        (x + width) << 2, (y + height - 4) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0, 0
    );

    gSPTextureRectangle(
        dl++,
        x << 2, (y + height - 4) << 2,
        (x + 4) << 2, (y + height) << 2,
        G_TX_RENDERTILE,
        0 << 5, 4 << 5,
        0x400, 0x400
    );

    gSPTextureRectangle(
        dl++,
        (x + 4) << 2, (y + height - 4) << 2,
        (x + width - 4) << 2, (y + height) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0, 0
    );

    gSPTextureRectangle(
        dl++,
        (x + width - 4) << 2, (y + height - 4) << 2,
        (x + width) << 2, (y + height) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0x400, 0x400
    );

    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuBuildHorizontalLine(int x, int y, int width) {
    Gfx* result = malloc(sizeof(Gfx) * 7);

    Gfx* dl = result;
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 86, 86, 86, 128);
    gDPFillRectangle(dl++, x, y, x + width, y + 1);
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 193, 193, 193, 128);
    gDPFillRectangle(dl++, x, y + 1, x + width, y + 2);
    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuRerenderSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh, Gfx* dl) {
    gDPFillRectangle(dl++, x, y, x + w, ny);
    gDPFillRectangle(dl++, x, ny, nx, ny + nh);
    gDPFillRectangle(dl++, nx + nw, ny, x + w, ny + nh);
    gDPFillRectangle(dl++, x, ny + nh, x + w, y + h);
    return dl;
}

Gfx* menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh) {
    Gfx* result = malloc(sizeof(Gfx) * 5);
    Gfx* dl = menuRerenderSolidBorder(x, y, w, h, nx, ny, nw, nh, result);

    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuRenderOutline(int x, int y, int width, int height, int invert, Gfx* dl) {
    gDPPipeSync(dl++);
    if (invert) {
        gDPSetEnvColor(dl++, gBorderDark.r, gBorderDark.g, gBorderDark.b, gBorderDark.a);
    } else {
        gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    }
    gDPFillRectangle(dl++, x, y, x + width - 1, y + 1);
    gDPFillRectangle(dl++, x, y, x + 1, y + height);
    gDPPipeSync(dl++);
    if (invert) {
        gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    } else {
        gDPSetEnvColor(dl++, gBorderDark.r, gBorderDark.g, gBorderDark.b, gBorderDark.a);
    }
    gDPFillRectangle(dl++, x, y + height - 1, x + width, y + height);
    gDPFillRectangle(dl++, x + width - 1, y, x + width, y + height - 1);

    return dl;
}

Gfx* menuBuildOutline(int x, int y, int width, int height, int invert) {
    Gfx* result = malloc(sizeof(Gfx) * 9);
    Gfx* dl = menuRenderOutline(x, y, width, height, invert, result);
    gSPEndDisplayList(dl++);
    return result;
}

struct MenuButton menuBuildButton(struct Font* font, char* message, int x, int y, int width, int height) {
    struct MenuButton result;

    result.text = menuBuildText(font, message, x + 4, y + 2);
    result.outline = menuBuildOutline(x, y, width, height, 0);

    result.x = x;
    result.y = y;

    result.w = width;
    result.h = height;

    return result;
}

void menuSetRenderColor(struct RenderState* renderState, int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor) {
    if (isSelected) {
        gDPSetEnvColor(renderState->dl++, selected->r, selected->g, selected->b, selected->a);
    } else {
        gDPSetEnvColor(renderState->dl++, defaultColor->r, defaultColor->g, defaultColor->b, defaultColor->a);
    }
}

#define CHECKBOX_SIZE   12

struct MenuCheckbox menuBuildCheckbox(struct Font* font, char* message, int x, int y) {
    struct MenuCheckbox result;

    result.x = x;
    result.y = y;

    result.outline = malloc(sizeof(Gfx) * 12);

    Gfx* dl = result.outline;

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 93, 96, 97, 255);
    gDPFillRectangle(dl++, x, y, x + CHECKBOX_SIZE, y + CHECKBOX_SIZE);
    dl = menuRenderOutline(x, y, CHECKBOX_SIZE, CHECKBOX_SIZE, 1, dl);
    gSPEndDisplayList(dl++);

    result.text = menuBuildText(font, message, x + CHECKBOX_SIZE + 6, y);

    result.checked = 0;

    return result;
}

Gfx* menuCheckboxRender(struct MenuCheckbox* checkbox, Gfx* dl) {
    if (!checkbox->checked) {
        return dl;
    }

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 255, 255, 255, 255);
    gDPFillRectangle(
        dl++, 
        checkbox->x + 3, 
        checkbox->y + 3, 
        checkbox->x + 8, 
        checkbox->y + 8
    );
    return dl;
}


#define SLIDER_TRACK_HEIGHT     4
#define SLIDER_HEIGHT           12
#define SLIDER_WIDTH            6
#define TICK_Y                  11
#define TICK_HEIGHT             3

struct MenuSlider menuBuildSlider(int x, int y, int w, int tickCount) {
    struct MenuSlider result;

    result.x = x;
    result.y = y;
    result.w = w;

    result.back = malloc(sizeof(Gfx) * (12 + tickCount));

    Gfx* dl = result.back;

    int sliderX = x;
    int sliderY = y + (SLIDER_HEIGHT / 2) - (SLIDER_TRACK_HEIGHT / 2);

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 25, 25, 25, 255);
    gDPFillRectangle(
        dl++, 
        sliderX, 
        sliderY, 
        sliderX + w, 
        sliderY + SLIDER_TRACK_HEIGHT
    );
    
    int tickMin = x + (SLIDER_WIDTH / 2);
    int tickWidth = w - SLIDER_WIDTH;
    for (int i = 0; i < tickCount; ++i) {
        int tickX = (i * tickWidth) / (tickCount - 1) + tickMin;
        gDPFillRectangle(
            dl++, 
            tickX, 
            y + TICK_Y, 
            tickX + 1, 
            y + TICK_Y + TICK_HEIGHT
        );  
    }

    dl = menuRenderOutline(sliderX, sliderY, w, SLIDER_TRACK_HEIGHT, 1, dl);

    gSPEndDisplayList(dl++);

    result.value = 0;

    return result;
}

Gfx* menuSliderRender(struct MenuSlider* slider, Gfx* dl) {
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 93, 96, 97, 255);

    int sliderPos = (slider->w - SLIDER_WIDTH) * slider->value + slider->x + (SLIDER_WIDTH / 2);

    gDPFillRectangle(
        dl++, 
        sliderPos - (SLIDER_WIDTH / 2), 
        slider->y, 
        sliderPos + (SLIDER_WIDTH / 2), 
        slider->y + SLIDER_HEIGHT
    );
    dl = menuRenderOutline(sliderPos - (SLIDER_WIDTH / 2), slider->y, SLIDER_WIDTH, SLIDER_HEIGHT, 0, dl);

    return dl;
}