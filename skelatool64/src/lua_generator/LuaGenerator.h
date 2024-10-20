#ifndef __LUA_GENERATOR_H__
#define __LUA_GENERATOR_H__

#include <lua.hpp>
#include <assimp/scene.h>
#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"
#include "../definition_generator/DefinitionGenerator.h"

bool checkLuaError(lua_State *L, int errCode, const char* filename);

void generateFromLuaScript(
    const std::string& levelFilename,
    const std::string& filename,
    const aiScene* scene,
    CFileDefinition& fileDefinition,
    NodeGroups& nodeGroups,
    const DisplayListSettings& settings
);

#endif