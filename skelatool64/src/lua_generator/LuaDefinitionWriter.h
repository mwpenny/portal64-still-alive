#ifndef __LUA_DEFINITON_WRITER_H__
#define __LUA_DEFINITON_WRITER_H__

#include <lua5.4/lua.hpp>
#include "../CFileDefinition.h"

bool dumpDefinitions(lua_State* L, CFileDefinition& fileDef, const char* filename);

void populateLuaDefinitionWrite(lua_State* L, CFileDefinition& fileDef);

#endif