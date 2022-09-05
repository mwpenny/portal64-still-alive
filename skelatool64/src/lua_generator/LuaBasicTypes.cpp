#include "LuaBasicTypes.h"

void toLua(lua_State* L, const std::string& string) {
    lua_pushlstring(L, string.c_str(), string.size());
}

void fromLua(lua_State* L, std::string& string) {
    size_t len;
    const char* str = lua_tolstring(L, -1, &len);
    string.assign(str, len);
    lua_pop(L, 1);
}

void toLua(lua_State* L, int number) {
    lua_pushinteger(L, number);
}

void fromLua(lua_State* L, int& number) {
    number = lua_tointeger(L, -1);
    lua_pop(L, 1);
}

void toLua(lua_State* L, double number) {
    lua_pushnumber(L, number);
}

void fromLua(lua_State* L, double& number) {
    number = lua_tonumber(L, -1);
    lua_pop(L, 1);
}