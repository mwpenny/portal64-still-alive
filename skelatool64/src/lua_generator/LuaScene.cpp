// docs for this file are located in sk_scene.lua

#include "LuaScene.h"

#include "LuaBasicTypes.h"
#include "LuaTransform.h"
#include "LuaMesh.h"
#include "LuaUtils.h"
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

    return (gLuaCurrentSettings->CreateCollisionTransform()) * result;
}

void luaGetMeshByIndex(lua_State* L, int indexAt0) {
    meshToLua(L, gLuaCurrentFileDefinition->GetExtendedMesh(gLuaCurrentScene->mMeshes[indexAt0]));
}

int luaNodeToString(lua_State* L) {
    lua_getfield(L, 1, "ptr");

    aiNode* node = (aiNode*)lua_touserdata(L, -1);

    std::ostringstream result;
    result << "aiNode: name=" << node->mName.C_Str();

    lua_pop(L, 1);
    lua_pushstring(L, result.str().c_str());

    return 1;
}

void toLua(lua_State* L, const aiVectorKey& positionKey) {
    lua_newtable(L);
    int tableIndex = lua_gettop(L);

    toLua(L, positionKey.mTime);
    lua_setfield(L, tableIndex, "time");

    toLua(L, positionKey.mValue);
    lua_setfield(L, tableIndex, "value");
}

void toLua(lua_State* L, const aiQuatKey& quatKey) {
    lua_newtable(L);
    int tableIndex = lua_gettop(L);

    toLua(L, quatKey.mTime);
    lua_setfield(L, tableIndex, "time");

    toLua(L, quatKey.mValue);
    lua_setfield(L, tableIndex, "value");
}

void toLua(lua_State* L, const aiNodeAnim* channel) {
    if (!channel) {
        lua_pushnil(L);
        return;
    }

    lua_newtable(L);
    int tableIndex = lua_gettop(L);

    toLua(L, channel->mNodeName.C_Str());
    lua_setfield(L, tableIndex, "node_name");

    toLua(L, channel->mPositionKeys, channel->mNumPositionKeys);
    lua_setfield(L, tableIndex, "position_keys");

    toLua(L, channel->mRotationKeys, channel->mNumRotationKeys);
    lua_setfield(L, tableIndex, "rotation_keys");

    toLua(L, channel->mScalingKeys, channel->mNumScalingKeys);
    lua_setfield(L, tableIndex, "scaling_keys");
}

void toLua(lua_State* L, const aiAnimation* animation) {
    if (!animation) {
        lua_pushnil(L);
        return;
    }

    lua_newtable(L);
    int tableIndex = lua_gettop(L);

    toLua(L, animation->mName.C_Str());
    lua_setfield(L, tableIndex, "name");

    toLua(L, animation->mDuration);
    lua_setfield(L, tableIndex, "duration");

    toLua(L, animation->mTicksPerSecond);
    lua_setfield(L, tableIndex, "ticks_per_second");

    toLua(L, animation->mChannels, animation->mNumChannels);
    lua_setfield(L, tableIndex, "channels");
}

void toLua(lua_State* L, const aiNode* node) {
    if (!node) {
        lua_pushnil(L);
        return;
    }

    // check if node ia already built in the cache
    lua_getglobal(L, "__sk_scene_node_cache__");
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
    
    if (luaL_newmetatable(L, "aiNode")) {
        lua_pushcfunction(L, luaNodeToString);
        lua_setfield(L, -2, "__tostring");
    }

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

    toLua(L, scene->mAnimations, scene->mNumAnimations);
    lua_setfield(L, tableIndex, "animations");
}

int luaExportDefaultMesh(lua_State* L) {
    const aiScene* scene = (const aiScene*)lua_touserdata(L, lua_upvalueindex(1));
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(2));
    std::shared_ptr<MeshDefinitionGenerator>* meshGenerator = (std::shared_ptr<MeshDefinitionGenerator>*)lua_touserdata(L, lua_upvalueindex(3));

    std::cout << "Generating mesh definitions" << std::endl;
    MeshDefinitionResults results = (*meshGenerator)->GenerateDefinitionsWithResults(scene, *fileDefinition);

    toLua(L, results.modelName);
    toLua(L, results.materialMacro);

    return 2;
}

int luaSceneAppendModule(lua_State* L) {
    int moduleIndex = luaGetPrevModuleLoader(L);

    std::shared_ptr<MeshDefinitionGenerator> meshGenerator(new MeshDefinitionGenerator(*gLuaCurrentSettings));
    meshGenerator->TraverseScene(gLuaCurrentScene);
    meshGenerator->PopulateBones(gLuaCurrentScene, *gLuaCurrentFileDefinition);

    lua_newtable(L);
    lua_setglobal(L, "__sk_scene_node_cache__");

    toLua(L, gLuaCurrentScene, *gLuaCurrentFileDefinition);
    lua_setfield(L, moduleIndex, "scene");

    lua_pushlightuserdata(L, const_cast<aiScene*>(gLuaCurrentScene));
    lua_pushlightuserdata(L, gLuaCurrentFileDefinition);
    toLua(L, meshGenerator);
    lua_pushcclosure(L, luaExportDefaultMesh, 3);
    lua_setfield(L, moduleIndex, "export_default_mesh");

    return 1;
}

void populateLuaScene(lua_State* L, const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings) {
    gLuaCurrentScene = scene;
    gLuaCurrentFileDefinition = &fileDefinition;
    gLuaCurrentSettings = &settings;

    luaChainModuleLoader(L, "sk_scene", luaSceneAppendModule, 0);
}