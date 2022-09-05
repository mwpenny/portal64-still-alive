#include "LuaTransform.h"


void toLua(lua_State* L, const aiMatrix4x4& matrix) {
    aiMatrix4x4* result = (aiMatrix4x4*)lua_newuserdata(L, sizeof(aiMatrix4x4));
    new(result) aiMatrix4x4(matrix);

    luaL_getmetatable(L, "aiMatrix4x4");
    lua_setmetatable(L, -2);
}

void generateLuaTransform(lua_State* L) {
    luaL_newmetatable(L, "aiMatrix4x4");
    lua_pop(L, 1);
}