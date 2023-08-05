/// @module sk_mesh

#include "LuaMesh.h"

#include <iomanip>

#include "../definition_generator/MeshDefinitionGenerator.h"
#include "../definition_generator/MaterialGenerator.h"
#include "./LuaBasicTypes.h"
#include "./LuaScene.h"
#include "../MeshWriter.h"
#include "./LuaDisplayListSettings.h"

#include "./LuaGeometry.h"
#include "./LuaTransform.h"
#include "./LuaUtils.h"

template <typename T>
int luaGetVector3ArrayElement(lua_State* L) {
    struct LazyVectorWithLength<T>* array = (struct LazyVectorWithLength<T>*)luaL_checkudata(L, 1, luaGetVectorName<T>().c_str());
    int index = luaL_checkinteger(L, 2);

    if (index <= 0 || index > array->length) {
        lua_pushnil(L);
        return 1;
    }

    toLua(L, array->vertices[index - 1]);

    return 1;
}

template <typename T>
int luaSetVector3ArrayElement(lua_State* L) {
    lua_settop(L, 3);
    
    T value;
    fromLua(L, value);

    struct LazyVectorWithLength<T>* array = (struct LazyVectorWithLength<T>*)luaL_checkudata(L, 1, luaGetVectorName<T>().c_str());
    int index = luaL_checkinteger(L, 2);

    if (index <= 0 || index > array->length) {
        return 0;
    }

    array->vertices[index - 1] = value;

    return 0;
}

template <typename T>
int luaGetVector3ArrayLength(lua_State* L) {
    struct LazyVectorWithLength<T>* array = (struct LazyVectorWithLength<T>*)luaL_checkudata(L, 1, luaGetVectorName<T>().c_str());
    lua_pushinteger(L, array->length);
    return 1;
}

template <typename T>
int luaVector3ArrayNext(lua_State* L) {
    struct LazyVectorWithLength<T>* array = (struct LazyVectorWithLength<T>*)luaL_checkudata(L, 1, luaGetVectorName<T>().c_str());

    if (lua_isnil(L, 2)) {
        if (array->length) {
            lua_pushinteger(L, 1);
            toLua(L, array->vertices[0]);
            return 2;
        }

        lua_pushnil(L);
        return 1;
    }

    if (lua_isinteger(L, 2)) {
        int current = lua_tointeger(L, 2);

        if (array->length <= current) {
            lua_pushnil(L);
            return 1;
        }

        lua_pushinteger(L, current + 1);
        toLua(L, array->vertices[current]);
        return 2;
    }

    lua_pushnil(L);
    return 1;
}

template <typename T>
int luaVector3ArrayPairs(lua_State* L) {
    lua_pushcfunction(L, luaVector3ArrayNext<T>);
    lua_pushnil(L);
    lua_copy(L, 1, -1);
    lua_pushnil(L);
    return 3;
}

template <typename T>
void toLuaLazyArray(lua_State* L, T* vertices, unsigned count) {
    struct LazyVectorWithLength<T>* result = (struct LazyVectorWithLength<T>*)lua_newuserdata(L, sizeof(struct LazyVectorWithLength<T>));

    result->vertices = vertices;
    result->length = count;

    if(luaL_newmetatable(L, luaGetVectorName<T>().c_str())) {
        lua_pushcfunction(L, luaGetVector3ArrayElement<T>);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, luaSetVector3ArrayElement<T>);
        lua_setfield(L, -2, "__newindex");

        lua_pushcfunction(L, luaGetVector3ArrayLength<T>);
        lua_setfield(L, -2, "__len");

        lua_pushcfunction(L, luaVector3ArrayPairs<T>);
        lua_setfield(L, -2, "__pairs");
    }

    lua_setmetatable(L, -2);
}

void textureFromLua(lua_State* L, std::shared_ptr<TextureDefinition>& texture) {
    lua_getfield(L, -1, "ptr");

    fromLua(L, texture);
    lua_pop(L, 1);
}

void textureToLua(lua_State* L, std::shared_ptr<TextureDefinition> texture);

int luaTextureCrop(lua_State* L) {
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);

    lua_settop(L, 1);

    std::shared_ptr<TextureDefinition> texture;
    textureFromLua(L, texture);

    std::shared_ptr<TextureDefinition> result = texture->Crop(x, y, w, h);
    textureToLua(L, result);
    return 1;
}

int luaTextureResize(lua_State* L) {
    int w = luaL_checkinteger(L, 2);
    int h = luaL_checkinteger(L, 3);

    lua_settop(L, 1);

    std::shared_ptr<TextureDefinition> texture;
    textureFromLua(L, texture);

    std::shared_ptr<TextureDefinition> result = texture->Resize(w, h);
    textureToLua(L, result);

    return 1;
}

int luaTextureData(lua_State* L) {
    lua_settop(L, 1);

    if (lua_isnil(L, 1)) {
        lua_pushstring(L, "call to get_data had nil as the first paramter");
        lua_error(L);
        return 0;
    }

    std::shared_ptr<TextureDefinition> texture;
    textureFromLua(L, texture);

    if (!texture) {
        return 0;
    }

    const std::vector<unsigned long long>& data = texture->GetData();
    
    lua_createtable(L, data.size(), 0);

    for (unsigned i = 0; i < data.size(); ++i) {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << data[i];
        luaLoadModuleFunction(L, "sk_definition_writer", "raw");
        lua_pushstring(L, stream.str().c_str());
        lua_call(L, 1, 1);
        lua_seti(L, -2, i + 1);
    }

    return 1;
}

/**
@table Texture
@tfield number width
@tfield number height
@tfield string name
@tfield function get_data
@tfield function crop
*/

void textureToLua(lua_State* L, std::shared_ptr<TextureDefinition> texture) {
    if (!texture) {
        lua_pushnil(L);
        return;
    }

    lua_createtable(L, 0, 0);

    luaLoadModuleFunction(L, "sk_mesh", "Texture");
    lua_setmetatable(L, -2);

    toLua(L, texture);
    lua_setfield(L, -2, "ptr");

    lua_pushinteger(L, texture->Width());
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, texture->Height());
    lua_setfield(L, -2, "height");

    lua_pushstring(L, texture->Name().c_str());
    lua_setfield(L, -2, "name");
}

/**
@table TileState
@tfield string format
@tfield string size
@tfield Texture texture 
*/
void toLua(lua_State* L, TileState& tileState) {
    lua_createtable(L, 0, 0);

    lua_pushstring(L, nameForImageFormat(tileState.format));
    lua_setfield(L, -2, "format");

    lua_pushstring(L, nameForImageSize(tileState.size));
    lua_setfield(L, -2, "size");

    textureToLua(L, tileState.texture);
    lua_setfield(L, -2, "texture");
}

/***
 @table Material
 @tfield string name
 @tfield string macro_name
 @tfield {...TileState} tiles
 */
void toLua(lua_State* L, Material* material) {
    if (!material) {
        lua_pushnil(L);
        return;
    }

    lua_createtable(L, 0, 0);

    luaLoadModuleFunction(L, "sk_mesh", "Material");
    lua_setmetatable(L, -2);

    toLua(L, material->mName);
    lua_setfield(L, -2, "name");

    toLua(L, MaterialGenerator::MaterialIndexMacroName(material->mName));
    lua_setfield(L, -2, "macro_name");

    lua_createtable(L, 0, material->mProperties.size());
    for (auto it : material->mProperties) {
        toLua(L, it.second);
        lua_setfield(L, -2, it.first.c_str());
    }
    lua_setfield(L, -2, "properties");

    lua_createtable(L, MAX_TILE_COUNT, 0);
    for (int i = 0; i < MAX_TILE_COUNT; ++i) {
        toLua(L, material->mState.tiles[i]);
        lua_seti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "tiles");

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

void toLua(lua_State* L, const aiFace& face) {
    lua_createtable(L, face.mNumIndices, 0);
    
    for (unsigned i = 0; i < face.mNumIndices; ++i) {
        toLua(L, face.mIndices[i] + 1);
        lua_seti(L, -2, i + 1);
    }
}

int luaTransformMesh(lua_State* L) {
    lua_settop(L, 2);

    aiMatrix4x4 transform;
    fromLua(L, transform);

    std::shared_ptr<ExtendedMesh> mesh;
    meshFromLua(L, mesh);

    std::shared_ptr<ExtendedMesh> result = mesh->Transform(transform);

    meshToLua(L, result);
    return 1;
}

/***
 @table Mesh
 @tfield string name
 @tfield bb sk_math.Box3
 @tfield sk_transform.Transform transform
 @tfield {sk_math.Vector3,...} vertices
 @tfield {sk_math.Vector3,...} normals
 @tfield {sk_math.Vector3,...} uv
 @tfield {{number,number,number},...} faces
 @tfield Material material
 */

void meshToLua(lua_State* L, std::shared_ptr<ExtendedMesh> mesh) {
    lua_createtable(L, 0, 1);
    
    luaLoadModuleFunction(L, "sk_mesh", "Mesh");
    lua_setmetatable(L, -2);

    toLua(L, mesh);
    lua_setfield(L, -2, "ptr");

    toLua(L, mesh->mMesh->mName.C_Str());
    lua_setfield(L, -2, "name");

    toLua(L, aiAABB(mesh->bbMin, mesh->bbMax));
    lua_setfield(L, -2, "bb");

    lua_pushcfunction(L, luaTransformMesh);
    lua_setfield(L, -2, "transform");

    toLuaLazyArray<aiVector3D>(L, mesh->mMesh->mVertices, mesh->mMesh->mNumVertices);
    lua_setfield(L, -2, "vertices");

    toLuaLazyArray<aiVector3D>(L, mesh->mMesh->mNormals, mesh->mMesh->mNumVertices);
    lua_setfield(L, -2, "normals");

    if (mesh->mMesh->mTextureCoords[0]) {
        toLuaLazyArray<aiVector3D>(L, mesh->mMesh->mTextureCoords[0], mesh->mMesh->mNumVertices);
    } else {
        lua_pushnil(L);
    }
    lua_setfield(L, -2, "uv");

    toLua(L, mesh->mMesh->mFaces, mesh->mMesh->mNumFaces);
    lua_setfield(L, -2, "faces");

    lua_createtable(L, AI_MAX_NUMBER_OF_COLOR_SETS, 0);
    for (int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
        if (!mesh->mMesh->mColors[i]) {
            mesh->mMesh->mColors[i] = new aiColor4D[mesh->mMesh->mNumVertices];

            for (unsigned vertexIndex = 0; vertexIndex < mesh->mMesh->mNumVertices; ++vertexIndex) {
                mesh->mMesh->mColors[i][vertexIndex] = aiColor4D(1.0, 1.0, 1.0, 1.0);
            }
        }
        toLuaLazyArray<aiColor4D>(L, mesh->mMesh->mColors[i], mesh->mMesh->mNumVertices);
        lua_seti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "colors");

    if (mesh->mMesh->mMaterialIndex >= 0 && mesh->mMesh->mMaterialIndex < gLuaCurrentScene->mNumMaterials) {
        auto material = gLuaCurrentSettings->mMaterials.find(gLuaCurrentScene->mMaterials[mesh->mMesh->mMaterialIndex]->GetName().C_Str());

        if (material != gLuaCurrentSettings->mMaterials.end()) {
            toLua(L, material->second.get());
            lua_setfield(L, -2, "material");
        }
    }

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

/***
 @table RenderChunk
 @tfield {Bone,Bone} bone_pair
 @tfield Mesh mesh
 @tfield sk_scene.Node meshRoot
 @tfield number attached_dl_index
 @tfield Material material
 */

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

/***
 generate runder chunks for a node
 @function generate_render_chunks
 @tparam sk_scene.Node node
 @treturn {RenderChunk,...} result
 */

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

/*** 
Generates a mesh
@function generate_mesh
@tparam {RenderChunk,...} renderChunks
@tparam string file suffix where the mesh definition is written to
@tparam[opt] DisplayListOverrides changes to the display list
 */
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

/***
 Generates a vertex buffer for a given mesh and material pair
 Materials are important since it will determine if the vertex
 buffer uses normals or colors and the size of the texture
 used for calculated uv coordinates
 @function generate_vertex_buffer
 @tparam Mesh mesh
 @tparam[opt] Material material
 @tparam[opt] string file_suffix defaults to "_geo"
 */
int luaGetMeshVertexBuffer(lua_State* L) {
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(1));

    int nArgs = lua_gettop(L);

    if (nArgs > 3) {
        lua_settop(L, 3);
        nArgs = 3;
    }

    std::string suffix;

    if (nArgs == 3) {
        fromLua(L, suffix);
        --nArgs;
    } else {
        suffix = "_geo";
    }

    Material* material = nullptr;

    if (nArgs == 2) {
        fromLua(L, material);
    }

    std::shared_ptr<ExtendedMesh> mesh;
    meshFromLua(L, mesh);


    std::string result = fileDefinition->GetVertexBuffer(
        mesh, 
        Material::GetVertexType(material), 
        Material::TextureWidth(material), 
        Material::TextureHeight(material), 
        suffix,
        material->mDefaultVertexColor
    );

    luaLoadModuleFunction(L, "sk_definition_writer", "raw");
    toLua(L, result);
    lua_call(L, 1, 1);

    return 1;
}

int buildMeshModule(lua_State* L) {
    aiScene* scene = (aiScene*)lua_touserdata(L, lua_upvalueindex(1));
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(2));
    DisplayListSettings* settings = (DisplayListSettings*)lua_touserdata(L, lua_upvalueindex(3));

    lua_newtable(L);

    lua_newtable(L);
    lua_setfield(L, -2, "Material");

    lua_newtable(L);
    lua_setfield(L, -2, "Mesh");

    lua_newtable(L);

    lua_pushcfunction(L, luaTextureCrop);
    lua_setfield(L, -2, "crop");

    lua_pushcfunction(L, luaTextureResize);
    lua_setfield(L, -2, "resize");

    lua_pushcfunction(L, luaTextureData);
    lua_setfield(L, -2, "get_data");
    
    lua_pushnil(L);
    lua_copy(L, -2, -1);
    lua_setfield(L, -2, "__index");

    lua_setfield(L, -2, "Texture");

    lua_pushlightuserdata(L, scene);
    lua_pushlightuserdata(L, fileDefinition);
    lua_pushlightuserdata(L, settings);
    lua_pushcclosure(L, luaBuildRenderChunks, 3);
    lua_setfield(L, -2, "generate_render_chunks");

    lua_pushlightuserdata(L, scene);
    lua_pushlightuserdata(L, fileDefinition);
    lua_pushlightuserdata(L, settings);
    lua_pushcclosure(L, luaGenerateMesh, 3);
    lua_setfield(L, -2, "generate_mesh");

    lua_pushlightuserdata(L, fileDefinition);
    lua_pushcclosure(L, luaGetMeshVertexBuffer, 1);
    lua_setfield(L, -2, "generate_vertex_buffer");

    return 1;
}

void populateLuaMesh(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings) {
    lua_pushlightuserdata(L, const_cast<aiScene*>(scene));
    lua_pushlightuserdata(L, &fileDefinition);
    lua_pushlightuserdata(L, const_cast<DisplayListSettings*>(&settings));
    lua_pushcclosure(L, buildMeshModule, 3);
    luaSetModuleLoader(L, "sk_mesh");
}