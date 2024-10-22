#include "LuaYaml.h"

#include "yaml-cpp/yaml.h"
#include <stdlib.h>
#include <sstream>
#include "./LuaUtils.h"

void toLua(lua_State* L, const YAML::Node& node) {
    if (!node.IsDefined() || node.IsNull()) {
        lua_pushnil(L);
        return;
    }

    if (node.IsSequence()) {
        lua_createtable(L, node.size(), 0);

        for (unsigned i = 0; i < node.size(); ++i) {
            toLua(L, node[i]);
            lua_seti(L, -2, i + 1);
        }

        return;
    }

    if (node.IsMap()) {
        lua_createtable(L, 0, node.size());

        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string name = it->first.as<std::string>();
            toLua(L, node[name]);
            lua_setfield(L, -2, name.c_str());
        }

        return;
    }

    if (node.IsScalar()) {
        std::string asString = node.as<std::string>();

        if (asString == "true") {
            lua_pushboolean(L, true);
            return;
        }

        if (asString == "false") {
            lua_pushboolean(L, false);
            return;
        }

        std::istringstream asStream(asString);

        lua_Integer asInt;

        if (!(asStream >> asInt).fail()) {
            lua_pushinteger(L, asInt);
            return;
        }

        asStream = std::istringstream(asString);

        lua_Number asDouble;
        if (!(asStream >> asDouble).fail()) {
            lua_pushnumber(L, asDouble);
            return;
        }

        lua_pushstring(L, asString.c_str());
        return;
    }

    lua_pushnil(L);
}

bool luaIsSequence(lua_State* L, int idx) {
    if (idx < 0) {
        idx = lua_gettop(L) + idx + 1;
    }

    lua_pushnil(L);
    if (!lua_next(L, idx)) {
        // empty sequence
        return true;
    }

    // first key is not a number
    if (!lua_isnumber(L, -2)) {
        lua_pop(L, 2);
        return false;
    }

    lua_pop(L, 2);

    // check iteration after end of sequence
    lua_len(L, idx);
    if (lua_next(L, idx)) {
        // has additional key value pairs
        lua_pop(L, 2);
        return false;
    }
    
    return true;
}

void fromLua(lua_State* L, YAML::Node& node) {
    switch (lua_type(L, -1))
    {
    case LUA_TBOOLEAN:
        node = YAML::Node(lua_toboolean(L, -1) ? true : false);
        break;
    case LUA_TNUMBER:
        node = YAML::Node(lua_tonumber(L, -1));
        break;
    case LUA_TSTRING:
        node = YAML::Node(lua_tostring(L, -1));
        break;
    case LUA_TTABLE:
        if (luaIsSequence(L, -1)) {
            node = YAML::Node(YAML::NodeType::Sequence);
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                YAML::Node element;
                fromLua(L, element);
                node.push_back(element);
            }
        } else {
            node = YAML::Node(YAML::NodeType::Map);
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                YAML::Node element;
                fromLua(L, element);
                node[lua_tostring(L, -1)] = element;
            }
        }

        break;
    default:
        node = YAML::Node(YAML::NodeType::Null);
        break;
    }

    lua_pop(L, 1);
}

int luaYamlParse(lua_State* L) {
    const char* value = luaL_checkstring(L, 1);

    YAML::Node doc = YAML::Load(value);
    toLua(L, doc);
    return 1;
}

int luaYamlStringify(lua_State* L) {
    lua_settop(L, 1);
    YAML::Node doc;
    fromLua(L, doc);
    std::string result = YAML::Dump(doc);
    lua_pushstring(L, result.c_str());
    return 1;
}

int buildYamlModule(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, luaYamlParse);
    lua_setfield(L, -2, "parse");

    lua_pushcfunction(L, luaYamlStringify);
    lua_setfield(L, -2, "stringify");

    return 1;
}

void generateLuaYaml(lua_State* L) {
    lua_pushcfunction(L, buildYamlModule);
    luaSetModuleLoader(L, "yaml");
}