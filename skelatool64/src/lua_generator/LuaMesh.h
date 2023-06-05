#ifndef __LUA_MESH_H__
#define __LUA_MESH_H__

#include <lua5.4/lua.hpp>
#include <assimp/scene.h>
#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"

template <typename T>
struct LazyVectorWithLength {
    T* vertices;
    int length;
};

void fromLua(lua_State* L, Material *& material);
void meshToLua(lua_State* L, std::shared_ptr<ExtendedMesh> mesh);
void meshFromLua(lua_State* L, std::shared_ptr<ExtendedMesh>& mesh);

template <typename T>
std::string luaGetVectorName() {
    return std::string("LazyVectorWithLength<") + typeid(T).name() + std::string(">");
}

template <typename T>
bool luaIsLazyVector3DArray(lua_State* L, int index) {
    return luaL_testudata(L, index, luaGetVectorName<T>().c_str());
}

void populateLuaMesh(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings);

#endif