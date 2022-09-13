#include "LuaGenerator.h"
#include "../FileUtils.h"

#include "LuaDefinitionWriter.h"
#include "LuaTransform.h"
#include "LuaNodeGroups.h"
#include "LuaScene.h"
#include "LuaMesh.h"
#include "LuaDisplayListSettings.h"

#include <lua.hpp>
#include <iostream>

#define EMIT(name) extern const char _binary_build_lua_##name##_out_start[]; extern const char _binary_build_lua_##name##_out_end[];
#include "LuaFiles.h"
#undef EMIT

struct LuaFile {
    const char* start;
    const char* end;
    const char* name;
};

struct LuaFile luaFiles[] = {
#define EMIT(name) {_binary_build_lua_##name##_out_start, _binary_build_lua_##name##_out_end, "lua/" #name ".lua"},
#include "LuaFiles.h"
#undef EMIT
};

bool checkLuaError(lua_State *L, int errCode, const char* filename) {
    if (errCode != LUA_OK) {
        const char* error = lua_tostring(L, -1);

        if (error) {
            std::cerr << "Error running " << filename << ":" << std::endl << error << std::endl;
        } else {
            std::cerr << "Unknown error running " << filename;
        }

        return true;
    }

    return false;
}

void generateFromLuaScript(
    const std::string& filename,
    const aiScene* scene,
    CFileDefinition& fileDefinition,
    NodeGroups& nodeGroups,
    const DisplayListSettings& settings
) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    generateLuaTransform(L);
    populateLuaNodeGroups(L, nodeGroups);
    populateLuaMesh(L, scene, fileDefinition, settings);
    populateLuaDefinitionWrite(L, fileDefinition);
    populateDisplayListSettings(L, settings);

    for (unsigned i = 0; i < sizeof(luaFiles) / sizeof(*luaFiles); ++i) {
        struct LuaFile* file = &luaFiles[i];
        luaL_loadbuffer(L, file->start, file->end - file->start, file->name);

        int stackSize = lua_gettop(L);
        int errCode = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (checkLuaError(L, errCode, file->name)) {
            lua_close(L);
            return;
        }

        lua_settop(L, stackSize);
    }

    populateLuaScene(L, scene);

    std::string directory = DirectoryName(filename) + "/?.lua;";

    // set the directory of the script location as a search path
    lua_getglobal(L, "package");
    int packageLocation = lua_gettop(L);
    lua_pushstring(L, directory.c_str());
    lua_getfield(L, packageLocation, "path");
    lua_concat(L, 2);
    lua_setfield(L, packageLocation, "path");

    int errCode = luaL_dofile(L, filename.c_str());
    if (checkLuaError(L, errCode, filename.c_str())) {
        lua_close(L);
        exit(1);
        return;
    }

    dumpDefinitions(L, fileDefinition, filename.c_str());

    lua_close(L);
}