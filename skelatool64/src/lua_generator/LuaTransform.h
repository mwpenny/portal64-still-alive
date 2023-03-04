#ifndef __LUA_TRANSFORM_H__
#define __LUA_TRANSFORM_H__

#include <lua5.4/lua.hpp>
#include <assimp/scene.h>

#include "LuaGeometry.h"

void toLua(lua_State* L, const aiMatrix4x4& matrix);
void fromLua(lua_State* L, aiMatrix4x4& matrix);

void generateLuaTransform(lua_State* L);

#endif