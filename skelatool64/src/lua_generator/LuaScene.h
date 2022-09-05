#ifndef __LUA_SCENE_H__
#define __LUA_SCENE_H__

#include <assimp/scene.h>
#include <lua.hpp>

void toLua(lua_State* L, const aiNode* node);

void fromLua(lua_State* L, aiNode *& node);

void populateLuaScene(lua_State* L, const aiScene* scene);

#endif