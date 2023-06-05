#ifndef __LUA_GEOMETRY_H__
#define __LUA_GEOMETRY_H__

#include <lua5.4/lua.hpp>
#include <assimp/scene.h>

void toLua(lua_State* L, const aiQuaternion& quaternion);
void toLua(lua_State* L, const aiVector3D& vector);
void toLua(lua_State* L, const aiColor4D& color);
void toLua(lua_State* L, const aiAABB& box);

void fromLua(lua_State* L, aiVector3D& vector);
void fromLua(lua_State* L, aiQuaternion& quaternion);
void fromLua(lua_State* L, aiColor4D& color);

#endif