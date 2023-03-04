#include "LuaGenerator.h"
#include "../FileUtils.h"

#include "LuaDefinitionWriter.h"
#include "LuaTransform.h"
#include "LuaNodeGroups.h"
#include "LuaScene.h"
#include "LuaMesh.h"
#include "LuaDisplayListSettings.h"
#include "LuaBasicTypes.h"

#include <lua5.4/lua.hpp>
#include <iostream>

#define EMIT(name) extern const char _binary_build_lua_##name##_out_start[]; extern const char _binary_build_lua_##name##_out_end[];
#include "LuaFiles.h"
#undef EMIT

struct LuaFile {
    const char* start;
    const char* end;
    const char* name;
    const char* moduleName;
};

struct LuaFile luaFiles[] = {
#define EMIT(name) {_binary_build_lua_##name##_out_start, _binary_build_lua_##name##_out_end, "lua/" #name ".lua", #name},
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

int loadPrecompiledModule(lua_State* L) {
    const char* moduleName = lua_tostring(L, lua_upvalueindex(1));

    for (unsigned i = 0; i < sizeof(luaFiles) / sizeof(*luaFiles); ++i) {
        struct LuaFile* file = &luaFiles[i];

        if (strcmp(moduleName, file->moduleName) != 0) {
            continue;
        }

        luaL_loadbuffer(L, file->start, file->end - file->start, file->name);

        int errCode = lua_pcall(L, 0, 1, 0);

        if (checkLuaError(L, errCode, file->name)) {
            lua_close(L);
            exit(1);
            return 0;
        }

        return 1;
    }

    return 0;
}

void generateFromLuaScript(
    const std::string& levelFilename,
    const std::string& filename,
    const aiScene* scene,
    CFileDefinition& fileDefinition,
    NodeGroups& nodeGroups,
    const DisplayListSettings& settings
) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    generateLuaTransform(L);
    populateLuaMesh(L, scene, fileDefinition, settings);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    for (unsigned i = 0; i < sizeof(luaFiles) / sizeof(*luaFiles); ++i) {
        struct LuaFile* file = &luaFiles[i];
        lua_pushstring(L, file->moduleName);
        lua_pushcclosure(L, loadPrecompiledModule, 1);
        lua_setfield(L, -2, file->moduleName);
    }

    // pop package and preload
    lua_pop(L, 2);

    populateDisplayListSettings(L, settings, levelFilename);
    populateLuaDefinitionWrite(L, fileDefinition);
    populateLuaScene(L, scene, fileDefinition, settings);
    populateLuaNodeGroups(L, nodeGroups);

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

    if (!dumpDefinitions(L, fileDefinition, filename.c_str())) {
        lua_close(L);
        exit(1);
        return;
    }

    lua_close(L);
}