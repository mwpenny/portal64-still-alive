#ifndef __MENU_TRANSLATIONS_H__
#define __MENU_TRANSLATIONS_H__

void translationsLoad(int language);
void translationsReload(int language);
int translationsCurrentLanguage();

char* translationsGet(int message);

#endif