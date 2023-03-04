#ifndef __LUA_NODE_GROUPS_H__
#define __LUA_NODE_GROUPS_H__

#include <lua5.4/lua.hpp>
#include "../definition_generator/DefinitionGenerator.h"

void populateLuaNodeGroups(lua_State* L, NodeGroups& nodeGroups);

#endif