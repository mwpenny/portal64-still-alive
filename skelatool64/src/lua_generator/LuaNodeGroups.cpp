// lua docs defined in sk_scene.lua

#include "LuaNodeGroups.h"

#include "LuaBasicTypes.h"
#include "LuaScene.h"
#include "LuaUtils.h"

void toLua(lua_State* L, const NodeWithArguments& nodes) {
    lua_createtable(L, 0, 2);

    int tableIndex = lua_gettop(L);

    toLua(L, nodes.arguments);
    lua_setfield(L, tableIndex, "arguments");

    toLua(L, nodes.node);
    lua_setfield(L, tableIndex, "node");
}

int luaNodesForType(lua_State* L) {
    std::string prefix = luaL_checkstring(L, 1);
    NodeGroups* nodeGroups = (NodeGroups*)lua_touserdata(L, lua_upvalueindex(1));

    std::vector<NodeWithArguments> result = nodeGroups->NodesForType(prefix);
    toLua(L, result);
    return 1;
}

int luaNodeGroupsAppendModule(lua_State* L) {
    int moduleIndex = luaGetPrevModuleLoader(L);

    NodeGroups* nodeGroups = (NodeGroups*)lua_touserdata(L, lua_upvalueindex(2));

    lua_pushlightuserdata(L, nodeGroups);
    lua_pushcclosure(L, luaNodesForType, 1);
    lua_setfield(L, moduleIndex, "nodes_for_type");

    return 1;
}

void populateLuaNodeGroups(lua_State* L, NodeGroups& nodeGroups) {
    lua_pushlightuserdata(L, (NodeGroups*)&nodeGroups);
    luaChainModuleLoader(L, "sk_scene", luaNodeGroupsAppendModule, 1);
}