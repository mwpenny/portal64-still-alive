#include "tabs.h"

#include "../util/memory.h"
#include "./menu.h"

#define LEFT_TEXT_PADDING  4
#define RIGHT_TEXT_PADDING 16
#define TOP_TEXT_PADDING   3

#define TAB_HEIGHT 18

void tabsSetSelectedTab(struct Tabs* tabs, int index) {
    if (index < 0 || index >= tabs->tabCount) {
        return;
    }

    tabs->selectedTab = index;

    Gfx* dl = tabs->tabOutline;

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gBorderDark.r, gBorderDark.g, gBorderDark.b, gBorderDark.a);
    gDPFillRectangle(dl++, tabs->x, tabs->y + tabs->height - 1, tabs->x + tabs->width, tabs->y + tabs->height);
    gDPFillRectangle(dl++, tabs->x + tabs->width - 1, tabs->y + TAB_HEIGHT, tabs->x + tabs->width, tabs->y + tabs->height);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);

        gDPFillRectangle(dl++, tab->x + tab->width - 2, tabTop, tab->x + tab->width - 1, tabs->y + TAB_HEIGHT);

        fontRender(tabs->font, tabs->tabs[i].message, tab->x + LEFT_TEXT_PADDING, tabTop + TOP_TEXT_PADDING, tab->text);
    }

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    gDPFillRectangle(dl++, tabs->x, tabs->y + TAB_HEIGHT, tabs->x + 1, tabs->y + tabs->height);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);

        gDPFillRectangle(dl++, tab->x, tabTop, tab->x + 1, tabs->y + TAB_HEIGHT);
        gDPFillRectangle(dl++, tab->x, tabTop, tab->x + tab->width - 2, tabTop + 1);
    }

    struct TabRenderData* selectedTab = tabs->selectedTab < tabs->tabCount ? &tabs->tabRenderData[tabs->selectedTab] : NULL;

    if (selectedTab) {
        gDPFillRectangle(dl++, tabs->x, tabs->y + TAB_HEIGHT, selectedTab->x, tabs->y + TAB_HEIGHT + 1);
        gDPFillRectangle(dl++, selectedTab->x + selectedTab->width, tabs->y + TAB_HEIGHT, tabs->x + tabs->width, tabs->y + TAB_HEIGHT + 1);
    }

    gSPEndDisplayList(dl++);
}

void tabsInit(struct Tabs* tabs, struct Tab* tabList, int tabCount, struct Font* font, int x, int y, int width, int height) {
    tabs->tabs = tabList;
    tabs->tabCount = tabCount;
    tabs->font = font;
    tabs->width = width;
    tabs->height = height;
    tabs->x = x;
    tabs->y = y;
    tabs->tabOutline = malloc(sizeof(Gfx) * (10 + 3 * tabCount));

    tabs->tabRenderData = malloc(sizeof(struct TabRenderData) * tabCount);

    int currentX = x;

    for (int i = 0; i < tabCount; ++i) {
        struct Vector2s16 textSize = fontMeasure(font, tabList[i].message);
        tabs->tabRenderData[i].text = menuBuildText(font, tabList[i].message, currentX + LEFT_TEXT_PADDING, y + TOP_TEXT_PADDING);
        tabs->tabRenderData[i].width = textSize.x + LEFT_TEXT_PADDING + RIGHT_TEXT_PADDING;
        tabs->tabRenderData[i].x = currentX;

        currentX += tabs->tabRenderData[i].width;
    }

    tabs->selectedTab = -1;
    tabsSetSelectedTab(tabs, 0);
}

Gfx* tabsRenderText(struct Tabs* tabs, Gfx* dl) {
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gSelectionGray.r, gSelectionGray.g, gSelectionGray.b, gSelectionGray.a);

    for (int i = 0; i < tabs->tabCount; ++i) {
        if (i == tabs->selectedTab) {
            gDPSetEnvColor(dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);
        }

        gSPDisplayList(dl++, tabs->tabRenderData[i].text);

        if (i == tabs->selectedTab) {
            gDPPipeSync(dl++);
            gDPSetEnvColor(dl++, gSelectionGray.r, gSelectionGray.g, gSelectionGray.b, gSelectionGray.a);
        }
    }

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);

    return dl;
}