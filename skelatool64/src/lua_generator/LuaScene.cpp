/// Scene functions

#include "LuaScene.h"

#include "LuaBasicTypes.h"
#include "LuaTransform.h"
#include "LuaMesh.h"
#include <iostream>

#include "../definition_generator/MeshDefinitionGenerator.h"

const aiScene* gLuaCurrentScene;
CFileDefinition* gLuaCurrentFileDefinition;
const DisplayListSettings* gLuaCurrentSettings;


aiMatrix4x4 nodeFullTrasformation(lua_State* L, const aiNode* node) {
    aiMatrix4x4 result;

    while (node) {
        result = node->mTransformation * result;
        node = node->mParent;
    }

    lua_getglobal(L, "settings");
    lua_getfield(L, -1, "model_transform");

    aiMatrix4x4* modelTransform = (aiMatrix4x4*)lua_touserdata(L, -1);

    lua_pop(L, 2);

    return (*modelTransform) * result;
}

void luaGetMeshByIndex(lua_State* L, int indexAt0) {
    meshToLua(L, gLuaCurrentFileDefinition->GetExtendedMesh(gLuaCurrentScene->mMeshes[indexAt0]));
}

void toLua(lua_State* L, const aiNode* node) {
    if (!node) {
        lua_pushnil(L);
        return;
    }

    // check if node ia already built in the cache
    lua_getglobal(L, "node_cache");
    int nodeCache = lua_gettop(L);
    lua_pushlightuserdata(L, const_cast<aiNode*>(node));
    lua_gettable(L, nodeCache);

    if (!lua_isnil(L, -1)) {
        lua_remove(L, nodeCache);
        return;
    }
    lua_pop(L, 1);

    lua_createtable(L, 0, 1);
    int tableIndex = lua_gettop(L);
    luaL_getmetatable(L, "aiNode");
    lua_setmetatable(L, tableIndex);

    lua_pushlightuserdata(L, const_cast<aiNode*>(node));
    lua_setfield(L, tableIndex, "ptr");

    toLua(L, node->mName.C_Str());
    lua_setfield(L, tableIndex, "name");

    toLua(L, node->mTransformation);
    lua_setfield(L, tableIndex, "transformation");

    aiMatrix4x4 fullTransformation = nodeFullTrasformation(L, node);
    toLua(L, fullTransformation);
    lua_setfield(L, tableIndex, "full_transformation");

    // save node into the cache before building related nodes
    lua_pushlightuserdata(L, const_cast<aiNode*>(node));
    lua_pushnil(L);
    lua_copy(L, tableIndex, -1);
    lua_settable(L, nodeCache);

    toLua(L, node->mParent);
    lua_setfield(L, tableIndex, "parent");

    toLua(L, node->mChildren, node->mNumChildren);
    lua_setfield(L, tableIndex, "children");

    lua_createtable(L, node->mNumMeshes, 0);
    for (unsigned i = 0; i < node->mNumMeshes; ++i) {
        luaGetMeshByIndex(L, node->mMeshes[i]);
        lua_seti(L, -2, i + 1);
    }
    lua_setfield(L, tableIndex, "meshes");

    lua_remove(L, nodeCache);
}

void fromLua(lua_State* L, aiNode *& node) {
    lua_getfield(L, -1, "ptr");
    node = (aiNode*)lua_touserdata(L, -1);
    lua_pop(L, 2);
}

void toLua(lua_State* L, const aiCamera* camera) {
    lua_createtable(L, 0, 2);

    toLua(L, std::string(camera->mName.C_Str()));
    lua_setfield(L, -2, "name");

    toLua(L, camera->mPosition);
    lua_setfield(L, -2, "position");

    toLua(L, camera->mLookAt);
    lua_setfield(L, -2, "look_at");

    toLua(L, camera->mUp);
    lua_setfield(L, -2, "up");

    toLua(L, camera->mHorizontalFOV);
    lua_setfield(L, -2, "horizontal_fov");

    toLua(L, camera->mClipPlaneNear);
    lua_setfield(L, -2, "near_plane");

    toLua(L, camera->mClipPlaneFar);
    lua_setfield(L, -2, "far_plane");

    toLua(L, camera->mAspect);
    lua_setfield(L, -2, "aspect_ratio");

    aiVector3D right = camera->mUp ^ camera->mLookAt;

    aiMatrix3x3 rotation;

    rotation.a1 = right.x; rotation.b1 = camera->mUp.x; rotation.c1 = camera->mLookAt.x;
    rotation.a2 = right.y; rotation.b2 = camera->mUp.y; rotation.c2 = camera->mLookAt.y;
    rotation.a3 = right.z; rotation.b3 = camera->mUp.z; rotation.c3 = camera->mLookAt.z;

    aiMatrix4x4 translation;
    aiMatrix4x4::Translation(camera->mPosition, translation);

    aiMatrix4x4 localTransform = translation * aiMatrix4x4(rotation);

    toLua(L, localTransform);
    lua_setfield(L, -2, "local_transform");
}

void toLua(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition) {
    lua_createtable(L, 0, 2);
    int tableIndex = lua_gettop(L);
    luaL_getmetatable(L, "aiScene");
    lua_setmetatable(L, tableIndex);

    lua_pushlightuserdata(L, const_cast<aiScene*>(scene));
    lua_setfield(L, tableIndex, "ptr");

    toLua(L, scene->mRootNode);
    lua_setfield(L, tableIndex, "root");

    toLua(L, scene->mCameras, scene->mNumCameras);
    lua_setfield(L, tableIndex, "cameras");

    lua_createtable(L, scene->mNumMeshes, 0);
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        meshToLua(L, fileDefinition.GetExtendedMesh(scene->mMeshes[i]));
        lua_seti(L, -2, i + 1);
    }
    lua_setfield(L, tableIndex, "meshes");
}

/***
Generates mesh and animation data from the current scene
@function export_default_mesh
 */
int luaExportDefaultMesh(lua_State* L) {
    const aiScene* scene = (const aiScene*)lua_touserdata(L, lua_upvalueindex(1));
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(2));
    std::shared_ptr<MeshDefinitionGenerator>* meshGenerator = (std::shared_ptr<MeshDefinitionGenerator>*)lua_touserdata(L, lua_upvalueindex(3));

    std::cout << "Generating mesh definitions" << std::endl;
    MeshDefinitionResults results = (*meshGenerator)->GenerateDefinitionsWithResults(scene, *fileDefinition);
    

    lua_createtable(L, 0, 2);

    lua_getglobal(L, "raw");
    toLua(L, results.modelName);
    lua_call(L, 1, 1);
    lua_setfield(L, -2, "model");

    lua_getglobal(L, "raw");
    toLua(L, results.materialMacro);
    lua_call(L, 1, 1);
    lua_setfield(L, -2, "material");

    return 1;
}

void populateLuaScene(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings) {
    gLuaCurrentScene = scene;
    gLuaCurrentFileDefinition = &fileDefinition;
    gLuaCurrentSettings = &settings;

    std::shared_ptr<MeshDefinitionGenerator> meshGenerator(new MeshDefinitionGenerator(settings));
    meshGenerator->TraverseScene(scene);
    meshGenerator->PopulateBones(scene, fileDefinition);


    lua_newtable(L);
    lua_setglobal(L, "node_cache");

    toLua(L, scene, fileDefinition);
    lua_setglobal(L, "scene");

    lua_pushlightuserdata(L, const_cast<aiScene*>(scene));
    lua_pushlightuserdata(L, &fileDefinition);
    toLua(L, meshGenerator);
    lua_pushcclosure(L, luaExportDefaultMesh, 3);
    lua_setglobal(L, "export_default_mesh");
}