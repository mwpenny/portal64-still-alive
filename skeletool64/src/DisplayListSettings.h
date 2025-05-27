#ifndef _DISPLAY_LIST_SETTINGS_H
#define _DISPLAY_LIST_SETTINGS_H

#include <string>
#include <map>
#include <assimp/scene.h>
#include <memory>
#include "./materials/Material.h"
#include "./materials/MaterialState.h"

#define DEFAULT_MAX_OPTIMIZATION_ITERATIONS 1000

struct DisplayListSettings {
    DisplayListSettings();
    std::string mPrefix;
    int mVertexCacheSize;
    bool mHasTri2;
    float mFixedPointScale;
    float mModelScale;
    int mMaxMatrixDepth;
    int mMaxOptimizationIterations;
    bool mCanPopMultipleMatrices;
    float mTicksPerSecond;
    std::map<std::string, std::shared_ptr<Material>> mMaterials;
    std::string mDefaultMaterialName;
    std::string mForceMaterialName;
    std::string mForcePalette;
    MaterialState mDefaultMaterialState;
    aiQuaternion mRotateModel;
    bool mExportAnimation;
    bool mExportGeometry;
    bool mIncludeCulling;
    bool mBonesAsVertexGroups;
    bool mTargetCIBuffer;

    aiVector3D mSortDirection;

    aiMatrix4x4 CreateGlobalTransform() const;
    aiMatrix4x4 CreateCollisionTransform() const;

    bool NeedsTangents() const;
};

#endif