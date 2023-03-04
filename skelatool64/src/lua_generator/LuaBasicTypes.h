#ifndef __LUA_BASIC_TYPES_H__
#define __LUA_BASIC_TYPES_H__

#include <lua5.4/lua.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

void toLua(lua_State* L, const std::string& string);

void fromLua(lua_State* L, std::string& string);

void toLua(lua_State* L, int number);
void toLua(lua_State* L, unsigned int number);

void fromLua(lua_State* L, int& number);

void toLua(lua_State* L, double number);

void fromLua(lua_State* L, double& number);

void fromLua(lua_State* L, float& number);

template <typename T> void toLua(lua_State* L, const std::vector<T> vector) {
    lua_createtable(L, vector.size(), 0);

    int tableIndex = lua_gettop(L);

    int index = 1;

    for (const auto& element : vector) {
        toLua(L, element);
        lua_seti(L, tableIndex, index);
        ++index;
    }
}

template <typename T> void fromLua(lua_State* L, std::vector<T>& result) {
    lua_len(L, -1);

    int length;
    fromLua(L, length);
    result.resize(length);

    for (int i = 1; i <= length; ++i) {
        lua_geti(L, -1, i);
        fromLua(L, result[i-1]);
    }
    lua_pop(L, 1);
}

template <typename T> void toLua(lua_State* L, const T* array, unsigned count) {
    lua_createtable(L, count, 0);

    int tableIndex = lua_gettop(L);

    for (unsigned i = 0; i < count; ++i) {
        toLua(L, array[i]);
        lua_seti(L, tableIndex, i + 1);
    }
}

template <typename A, typename B> void toLua(lua_State* L, const std::pair<A, B>& pair) {
    lua_createtable(L, 2, 0);

    int tableIndex = lua_gettop(L);

    toLua(L, pair.first);
    lua_seti(L, tableIndex, 1);

    toLua(L, pair.second);
    lua_seti(L, tableIndex, 2);
}

template <typename A, typename B> void fromLua(lua_State* L, std::pair<A, B>& pair) {
    lua_geti(L, -1, 1);
    fromLua(L, pair.first);
    lua_geti(L, -1, 2);
    fromLua(L, pair.second);
    lua_pop(L, 1);
}

template <typename T> int luaSharedPtrGC(lua_State* L) {
    std::shared_ptr<T>* ptr = (std::shared_ptr<T>*)luaL_checkudata(L, 1, typeid(std::shared_ptr<T>).name());
    ptr->~shared_ptr();
    return 0;
}

template <typename T> void toLua(lua_State* L, std::shared_ptr<T> ptr) {
    std::shared_ptr<T>* result = (std::shared_ptr<T>*)lua_newuserdata(L, sizeof(std::shared_ptr<T>));

    int resultIndex = lua_gettop(L);

    new(result) std::shared_ptr<T>(ptr);

    if (luaL_newmetatable(L, typeid(std::shared_ptr<T>).name())) {
        lua_pushcfunction(L, luaSharedPtrGC<T>);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, resultIndex);
}

template <typename T> void fromLua(lua_State* L, std::shared_ptr<T>& output) {
    std::shared_ptr<T>* ptr = (std::shared_ptr<T>*)luaL_checkudata(L, -1, typeid(std::shared_ptr<T>).name());
    output = *ptr;
    lua_pop(L, 1);
}

#endif