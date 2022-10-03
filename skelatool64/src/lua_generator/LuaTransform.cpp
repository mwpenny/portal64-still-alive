#include "LuaTransform.h"

#include "LuaBasicTypes.h"

void toLua(lua_State* L, const aiMatrix4x4& matrix) {
    aiMatrix4x4* result = (aiMatrix4x4*)lua_newuserdata(L, sizeof(aiMatrix4x4));
    new(result) aiMatrix4x4(matrix);

    luaL_getmetatable(L, "aiMatrix4x4");
    lua_setmetatable(L, -2);
}


void fromLua(lua_State* L, aiMatrix4x4& matrix) {
    aiMatrix4x4* ptr = (aiMatrix4x4*)luaL_checkudata(L, -1, "aiMatrix4x4");
    if (ptr) {
        matrix = *ptr;
    }
    lua_pop(L, 1);
}

int luaTransformDecomponse(lua_State* L) {
    aiMatrix4x4* mtx = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D position;
    mtx->Decompose(scaling, rotation, position);

    toLua(L, scaling);
    toLua(L, rotation);
    toLua(L, position);

    return 3;
}

int luaTransformIndex(lua_State* L) {
    aiMatrix4x4* mtx = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    int keyType = lua_type(L, 2);

    if (keyType == LUA_TSTRING) {
        const char* key = lua_tostring(L, 2);

        if (strcmp(key, "decompose") == 0) {
            lua_pushcfunction(L, luaTransformDecomponse);
        } else {
            lua_pushnil(L);
        }

        return 1;
    } else if (keyType == LUA_TTABLE) {
        lua_len(L, 2);
        int length = lua_tointeger(L, -1);
        lua_pop(L, 1);

        if (length == 2) {
            lua_geti(L, 2, 1);
            lua_geti(L, 2, 2);

            int row = lua_tointeger(L, -2);
            int col = lua_tointeger(L, -1);

            lua_pushnumber(L, (*mtx)[row][col]);
            return 1;
        }
    }

    lua_pushfstring(L, "Expected a string key or an array of length 2 got '%s'", lua_typename(L, keyType));
    lua_error(L);
    return 0;
}

bool luaIsTransform(lua_State* L, int idx) {
    if (lua_type(L, idx) != LUA_TUSERDATA) {
        return false;
    }
    
    if (!lua_getmetatable(L, idx)) {
        return false;
    }

    luaL_getmetatable(L, "aiMatrix4x4");

    bool result = lua_rawequal(L, -2, -1);
    lua_pop(L, 2);
    return result;
}

int luaTransformMul(lua_State* L) {
    aiMatrix4x4* a = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    if (luaIsTransform(L, 2)) {
        aiMatrix4x4* b = (aiMatrix4x4*)luaL_checkudata(L, 2, "aiMatrix4x4");

        aiMatrix4x4 result = (*a) * (*b);

        toLua(L, result);

        return 1;
    }

    luaL_checkudata(L, 2, "aiMatrix4x4");
    return 0;
}

void generateLuaTransform(lua_State* L) {
    luaL_newmetatable(L, "aiMatrix4x4");

    lua_pushcfunction(L, luaTransformIndex);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, luaTransformMul);
    lua_setfield(L, -2, "__mul");

    lua_pop(L, 1);
}