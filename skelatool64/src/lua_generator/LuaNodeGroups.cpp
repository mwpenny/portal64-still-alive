#include "LuaNodeGroups.h"

#include "LuaBasicTypes.h"
#include "LuaScene.h"

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

void populateLuaNodeGroups(lua_State* L, NodeGroups& nodeGroups) {
    lua_pushlightuserdata(L, (NodeGroups*)&nodeGroups);
    lua_pushcclosure(L, luaNodesForType, 1);
    lua_setglobal(L, "nodes_for_type");
}