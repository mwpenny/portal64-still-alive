#ifndef __LUA_SCENE_H__
#define __LUA_SCENE_H__

#include <assimp/scene.h>
#include <lua5.4/lua.hpp>

#include "../DisplayListSettings.h"
#include "../CFileDefinition.h"

extern const aiScene* gLuaCurrentScene;
extern const DisplayListSettings* gLuaCurrentSettings;

void toLua(lua_State* L, const aiNode* node);

void fromLua(lua_State* L, aiNode *& node);

void populateLuaScene(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& setting);

#endif