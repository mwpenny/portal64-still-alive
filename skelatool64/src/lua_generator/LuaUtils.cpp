#include "LuaUtils.h"

void luaLoadModuleFunction(lua_State* L, const char* moduleName, const char* functionName) {
    lua_getglobal(L, "require");
    lua_pushstring(L, moduleName);
    lua_call(L, 1, 1);

    lua_getfield(L, -1, functionName);
    lua_remove(L, -2);
}

void luaSetModuleLoader(lua_State* L, const char* moduleName) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_rotate(L, -3, -1);
    lua_setfield(L, -2, moduleName);
    lua_pop(L, 2);
}

void luaChainModuleLoader(lua_State* L, const char* moduleName, lua_CFunction function, int additionalClosure) {
    int startingTop = lua_gettop(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    lua_getfield(L, -1, moduleName);

    if (additionalClosure) {
        lua_rotate(L, startingTop - additionalClosure + 1, -additionalClosure);
    }

    lua_pushcclosure(L, function, 1 + additionalClosure);
    lua_setfield(L, -2, moduleName);

    // pop package and preload
    lua_pop(L, 2);
}

int luaGetPrevModuleLoader(lua_State* L) {
    int nArgs = lua_gettop(L);

    lua_pushnil(L);
    lua_copy(L, lua_upvalueindex(1), -1);

    lua_insert(L, 1);
    lua_call(L, nArgs, 1);

    return lua_gettop(L);
}