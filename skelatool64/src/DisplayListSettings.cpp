#include "DisplayListSettings.h"

#include "RCPState.h"

DisplayListSettings::DisplayListSettings():
    mPrefix(""),
    mVertexCacheSize(MAX_VERTEX_CACHE_SIZE),
    mHasTri2(true),
    mGraphicsScale(256.0f),
    mCollisionScale(1.0f),
    mMaxMatrixDepth(10),
    mCanPopMultipleMatrices(true),
    mTicksPerSecond(30),
    mExportAnimation(true),
    mExportGeometry(true),
    mIncludeCulling(true) {
}

aiMatrix4x4 DisplayListSettings::CreateGlobalTransform() {
    aiMatrix4x4 scale;
    aiMatrix4x4::Scaling(aiVector3D(1, 1, 1) * mGraphicsScale, scale);
    aiMatrix4x4 rotation(mRotateModel.GetMatrix());
    
    return rotation * scale;
}