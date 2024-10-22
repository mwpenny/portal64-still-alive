#include "DisplayListSettings.h"

#include "RCPState.h"

DisplayListSettings::DisplayListSettings():
    mPrefix(""),
    mVertexCacheSize(MAX_VERTEX_CACHE_SIZE),
    mHasTri2(true),
    mFixedPointScale(256.0f),
    mModelScale(1.0f),
    mMaxMatrixDepth(10),
    mMaxOptimizationIterations(DEFAULT_MAX_OPTIMIZATION_ITERATIONS),
    mCanPopMultipleMatrices(true),
    mTicksPerSecond(30.0f),
    mExportAnimation(true),
    mExportGeometry(true),
    mIncludeCulling(true),
    mTargetCIBuffer(false) {
}

aiMatrix4x4 DisplayListSettings::CreateGlobalTransform() const {
    aiMatrix4x4 scale;
    aiMatrix4x4::Scaling(aiVector3D(1, 1, 1) * mFixedPointScale * mModelScale, scale);
    aiMatrix4x4 rotation(mRotateModel.GetMatrix());
    
    return rotation * scale;
}

aiMatrix4x4 DisplayListSettings::CreateCollisionTransform() const {
    aiMatrix4x4 scale;
    aiMatrix4x4::Scaling(aiVector3D(1, 1, 1) * mModelScale, scale);
    aiMatrix4x4 rotation(mRotateModel.GetMatrix());
    
    return rotation * scale;
}

bool DisplayListSettings::NeedsTangents() const {
    for (auto& material : mMaterials) {
        if (material.second->mNormalSource != NormalSource::Normal) {
            return true;
        }
    }

    return false;
}