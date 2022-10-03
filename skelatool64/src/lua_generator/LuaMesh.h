#ifndef __LUA_MESH_H__
#define __LUA_MESH_H__

#include <lua.hpp>
#include <assimp/scene.h>
#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"

struct aiVector3DArray {
    aiVector3D* vertices;
    int length;
};

void fromLua(lua_State* L, Material *& material);
void meshToLua(lua_State* L, std::shared_ptr<ExtendedMesh> mesh);
void meshFromLua(lua_State* L, std::shared_ptr<ExtendedMesh>& mesh);
bool luaIsLazyVector3DArray(lua_State* L, int index);

void populateLuaMesh(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings);

#endif