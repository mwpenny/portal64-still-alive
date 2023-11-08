#include "tabs.h"

#include "../util/memory.h"
#include "./translations.h"
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

    int tabOffset = 0;

    struct TabRenderData* rightVisibleTab = &tabs->tabRenderData[tabs->selectedTab];

    if (tabs->selectedTab + 1 < tabs->tabCount) {
        ++rightVisibleTab;
    }

    if (rightVisibleTab->x + rightVisibleTab->width >= tabs->x + tabs->width) {
        tabOffset = (tabs->x + tabs->width) - (rightVisibleTab->x + rightVisibleTab->width) + 1;
    }

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);
        int tabLeft = tab->x + tabOffset;

        gDPFillRectangle(dl++, tabLeft + tab->width - 2, tabTop, tabLeft + tab->width - 1, tabs->y + TAB_HEIGHT);

        prerenderedTextRelocate(tab->text, tabLeft + LEFT_TEXT_PADDING, tabTop + TOP_TEXT_PADDING);
    }

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    gDPFillRectangle(dl++, tabs->x, tabs->y + TAB_HEIGHT, tabs->x + 1, tabs->y + tabs->height);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);
        int tabLeft = tab->x + tabOffset;

        gDPFillRectangle(dl++, tabLeft, tabTop, tabLeft + 1, tabs->y + TAB_HEIGHT);
        gDPFillRectangle(dl++, tabLeft, tabTop, tabLeft + tab->width - 2, tabTop + 1);
    }

    struct TabRenderData* selectedTab = tabs->selectedTab < tabs->tabCount ? &tabs->tabRenderData[tabs->selectedTab] : NULL;

    if (selectedTab) {
        gDPFillRectangle(dl++, tabs->x, tabs->y + TAB_HEIGHT, selectedTab->x + tabOffset, tabs->y + TAB_HEIGHT + 1);
        gDPFillRectangle(dl++, selectedTab->x + tabOffset + selectedTab->width, tabs->y + TAB_HEIGHT, tabs->x + tabs->width, tabs->y + TAB_HEIGHT + 1);
    }

    gSPEndDisplayList(dl++);

    tabs->prevOffset = tabOffset;
}

void tabsInit(struct Tabs* tabs, struct Tab* tabList, int tabCount, struct Font* font, int x, int y, int width, int height) {
    tabs->tabs = tabList;
    tabs->tabCount = tabCount;
    tabs->font = font;
    tabs->width = width;
    tabs->height = height;
    tabs->x = x;
    tabs->y = y;
    tabs->prevOffset = 0;
    tabs->tabOutline = malloc(sizeof(Gfx) * (10 + 3 * tabCount));

    tabs->tabRenderData = malloc(sizeof(struct TabRenderData) * tabCount);

    for (int i = 0; i < tabCount; ++i) {
        tabs->tabRenderData[i].text = NULL;
    }

    tabs->selectedTab = 0;
    tabsRebuildText(tabs);
}

void tabsRenderText(struct Tabs* tabs, struct PrerenderedTextBatch* batch) {
    for (int i = 0; i < tabs->tabCount; ++i) {
        prerenderedBatchAdd(batch, tabs->tabRenderData[i].text, i == tabs->selectedTab ? &gColorWhite : &gSelectionGray);
    }
}

void tabsRebuildText(struct Tabs* tabs) {
    int currentX = tabs->x;

    for (int i = 0; i < tabs->tabCount; ++i) {
        prerenderedTextFree(tabs->tabRenderData[i].text);
        tabs->tabRenderData[i].text = menuBuildPrerenderedText(
            tabs->font, 
            translationsGet(tabs->tabs[i].messageId), 
            currentX + LEFT_TEXT_PADDING, 
            tabs->y + TOP_TEXT_PADDING,
            SCREEN_WD
        );
        tabs->tabRenderData[i].width = tabs->tabRenderData[i].text->width + LEFT_TEXT_PADDING + RIGHT_TEXT_PADDING;
        tabs->tabRenderData[i].x = currentX;

        currentX += tabs->tabRenderData[i].width;
    }

    int selectedTab = tabs->selectedTab;
    tabs->selectedTab = -1;
    tabsSetSelectedTab(tabs, selectedTab);
}