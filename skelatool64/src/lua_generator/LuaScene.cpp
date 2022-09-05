#include "LuaScene.h"

#include "LuaBasicTypes.h"
#include "LuaTransform.h"
#include <iostream>

void toLua(lua_State* L, const aiNode* node) {
    if (!node) {
        lua_pushnil(L);
        return;
    }

    // check if node ia already built in the cache
    lua_getglobal(L, "node_cache");
    int nodeCache = lua_gettop(L);
    lua_pushlightuserdata(L, const_cast<aiNode*>(node));
    lua_gettable(L, nodeCache);

    if (!lua_isnil(L, -1)) {
        lua_remove(L, nodeCache);
        return;
    }
    lua_pop(L, 1);

    lua_createtable(L, 0, 1);
    int tableIndex = lua_gettop(L);
    luaL_getmetatable(L, "aiNode");
    lua_setmetatable(L, tableIndex);

    lua_pushlightuserdata(L, const_cast<aiNode*>(node));
    lua_setfield(L, tableIndex, "ptr");

    toLua(L, node->mName.C_Str());
    lua_setfield(L, tableIndex, "name");

    toLua(L, node->mTransformation);
    lua_setfield(L, tableIndex, "transformation");

    // save node into the cache before building related nodes
    lua_pushlightuserdata(L, const_cast<aiNode*>(node));
    lua_pushnil(L);
    lua_copy(L, tableIndex, -1);
    lua_settable(L, nodeCache);

    toLua(L, node->mParent);
    lua_setfield(L, tableIndex, "parent");

    toLua(L, node->mChildren, node->mNumChildren);
    lua_setfield(L, tableIndex, "children");

    lua_remove(L, nodeCache);
}

void fromLua(lua_State* L, aiNode *& node) {
    lua_getfield(L, -1, "ptr");
    node = (aiNode*)lua_touserdata(L, -1);
    lua_pop(L, 2);
}

void toLua(lua_State* L, const aiScene* scene) {
    lua_createtable(L, 0, 1);
    int tableIndex = lua_gettop(L);
    luaL_getmetatable(L, "aiScene");
    lua_setmetatable(L, tableIndex);

    lua_pushlightuserdata(L, const_cast<aiScene*>(scene));
    lua_setfield(L, tableIndex, "ptr");

    toLua(L, scene->mRootNode);
    lua_setfield(L, tableIndex, "root");
}

void populateLuaScene(lua_State* L, const aiScene* scene) {
    lua_newtable(L);
    lua_setglobal(L, "node_cache");

    toLua(L, scene);
    lua_setglobal(L, "scene");
}