#ifndef __LUA_DISPLAY_LIST_SETTINGS_H__
#define __LUA_DISPLAY_LIST_SETTINGS_H__

#include <lua5.4/lua.hpp>
#include "../DisplayListSettings.h"

void fromLua(lua_State* L, DisplayListSettings& result, const DisplayListSettings& defaults);

void populateDisplayListSettings(lua_State* L, const DisplayListSettings& defaults, const std::string& levelFilename);

#endif