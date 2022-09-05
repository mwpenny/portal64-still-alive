#ifndef __LUA_MESH_H__
#define __LUA_MESH_H__

#include <lua.hpp>
#include <assimp/scene.h>
#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"

void fromLua(lua_State* L, Material *& material);

void populateLuaMesh(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings);

#endif