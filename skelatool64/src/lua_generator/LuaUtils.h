#ifndef __LUA_UTILS_H__
#define __LUA_UTILS_H__

#include <lua5.4/lua.hpp>

void luaLoadModuleFunction(lua_State* L, const char* moduleName, const char* functionName);

void luaSetModuleLoader(lua_State* L, const char* moduleName);

void luaChainModuleLoader(lua_State* L, const char* moduleName, lua_CFunction function, int additionalClosure);

int luaGetPrevModuleLoader(lua_State* L);

#endif