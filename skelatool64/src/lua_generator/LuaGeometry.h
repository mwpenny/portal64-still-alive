#ifndef __LUA_GEOMETRY_H__
#define __LUA_GEOMETRY_H__

#include <lua.hpp>
#include <assimp/scene.h>

void toLua(lua_State* L, const aiQuaternion& quaternion);
void toLua(lua_State* L, const aiVector3D& vector);
void toLua(lua_State* L, const aiAABB& box);

void fromLua(lua_State* L, aiVector3D& vector);

#endif