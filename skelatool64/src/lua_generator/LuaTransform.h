#ifndef __LUA_TRANSFORM_H__
#define __LUA_TRANSFORM_H__

#include <lua.hpp>
#include <assimp/scene.h>

void toLua(lua_State* L, const aiMatrix4x4& matrix);
void toLua(lua_State* L, const aiVector3D& vector);
void toLua(lua_State* L, const aiQuaternion& quaternion);

void generateLuaTransform(lua_State* L);

#endif