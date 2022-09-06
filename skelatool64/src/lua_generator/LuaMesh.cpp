#include "LuaMesh.h"

#include "../definition_generator/MeshDefinitionGenerator.h"
#include "../definition_generator/MaterialGenerator.h"
#include "./LuaBasicTypes.h"
#include "./LuaScene.h"
#include "../MeshWriter.h"
#include "./LuaDisplayListSettings.h"

void toLua(lua_State* L, Material* material) {
    if (!material) {
        lua_pushnil(L);
        return;
    }

    lua_createtable(L, 1, 0);

    luaL_getmetatable(L, "Material");
    lua_setmetatable(L, -2);

    toLua(L, material->mName);
    lua_setfield(L, -2, "name");

    toLua(L, MaterialGenerator::MaterialIndexMacroName(material->mName));
    lua_setfield(L, -2, "macro_name");

    lua_pushlightuserdata(L, material);
    lua_setfield(L, -2, "ptr");
}

void fromLua(lua_State* L, Material *& material) {
    if (lua_isnil(L, -1)) {
        material = nullptr;
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "ptr");
    material = (Material*)lua_touserdata(L, -1);
    lua_pop(L, 2);
}

void meshToLua(lua_State* L, std::shared_ptr<ExtendedMesh> mesh) {
    lua_createtable(L, 0, 1);

    toLua(L, mesh);
    lua_setfield(L, -2, "ptr");
}

void meshFromLua(lua_State* L, std::shared_ptr<ExtendedMesh>& mesh) {
    lua_getfield(L, -1, "ptr");

    fromLua(L, mesh);
    lua_pop(L, 1);
}

void toLua(lua_State* L, const Bone* bone) {
    lua_pushlightuserdata(L, const_cast<Bone*>(bone));
}

void fromLua(lua_State*L, Bone *& bone) {
    bone = (Bone*)lua_touserdata(L, -1);
    lua_pop(L, 1);
}

void toLua(lua_State* L, const RenderChunk& renderChunk) {
    lua_createtable(L, 0, 5);

    int tableIndex = lua_gettop(L);

    toLua(L, renderChunk.mBonePair);
    lua_setfield(L, tableIndex, "bone_pair");

    meshToLua(L, renderChunk.mMesh);
    lua_setfield(L, tableIndex, "mesh");

    toLua(L, renderChunk.mMeshRoot);
    lua_setfield(L, tableIndex, "meshRoot");

    toLua(L, renderChunk.mAttachedDLIndex);
    lua_setfield(L, tableIndex, "attached_dl_index");

    toLua(L, renderChunk.mMaterial);
    lua_setfield(L, tableIndex, "material");
}

void fromLua(lua_State* L, RenderChunk& result) {
    lua_getfield(L, -1, "bone_pair");
    fromLua(L, result.mBonePair);

    lua_getfield(L, -1, "mesh");
    meshFromLua(L, result.mMesh);

    lua_getfield(L, -1, "meshRoot");
    fromLua(L, result.mMeshRoot);

    lua_getfield(L, -1, "attached_dl_index");
    fromLua(L, result.mAttachedDLIndex);

    lua_getfield(L, -1, "material");
    fromLua(L, result.mMaterial);

    lua_pop(L, 1);
}

int luaBuildRenderChunks(lua_State* L) {
    const aiScene* scene = (const aiScene*)lua_touserdata(L, lua_upvalueindex(1));
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(2));
    DisplayListSettings* settings = (DisplayListSettings*)lua_touserdata(L, lua_upvalueindex(3));

    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "ptr");

    aiNode* node = (aiNode*)lua_touserdata(L, -1);

    std::vector<RenderChunk> renderChunks;
    if (node) {
        MeshDefinitionGenerator::AppendRenderChunks(scene, node, *fileDefinition, *settings, renderChunks);
    }

    toLua(L, renderChunks);

    return 1;
}

int luaGenerateMesh(lua_State* L) {
    const aiScene* scene = (const aiScene*)lua_touserdata(L, lua_upvalueindex(1));
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(2));
    DisplayListSettings* settings = (DisplayListSettings*)lua_touserdata(L, lua_upvalueindex(3));
    
    luaL_checktype(L, 1, LUA_TTABLE);
    const char* location = luaL_checkstring(L, 2);

    std::vector<RenderChunk> renderChunks;

    lua_pushnil(L);
    lua_copy(L, 1, -1);
    fromLua(L, renderChunks);

    std::string result;
    
    if (lua_gettop(L) >= 3 && lua_type(L, 3) == LUA_TTABLE) {
        lua_settop(L, 3);
        DisplayListSettings settingOverride;
        fromLua(L, settingOverride, *settings);

        result = generateMesh(scene, *fileDefinition, renderChunks, settingOverride, location);
    } else {
        result = generateMesh(scene, *fileDefinition, renderChunks, *settings, location);
    }

    toLua(L, result);
    return 1;
}

void populateLuaMesh(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings) {
    lua_newtable(L);
    luaL_setmetatable(L, "Material");

    lua_pushlightuserdata(L, const_cast<aiScene*>(scene));
    lua_pushlightuserdata(L, &fileDefinition);
    lua_pushlightuserdata(L, const_cast<DisplayListSettings*>(&settings));
    lua_pushcclosure(L, luaBuildRenderChunks, 3);
    lua_setglobal(L, "generate_render_chunks");

    lua_pushlightuserdata(L, const_cast<aiScene*>(scene));
    lua_pushlightuserdata(L, &fileDefinition);
    lua_pushlightuserdata(L, const_cast<DisplayListSettings*>(&settings));
    lua_pushcclosure(L, luaGenerateMesh, 3);
    lua_setglobal(L, "generate_mesh");
}