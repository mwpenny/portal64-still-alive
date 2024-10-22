#ifndef __ANIMATION_GENERATOR_H__
#define __ANIMATION_GENERATOR_H__

#include <assimp/scene.h>
#include <vector>
#include <memory>

#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"

struct AnimationResults {
    std::string initialPoseReference;
    std::string boneParentReference;
    std::string boneCountMacro;
    std::string numberOfAttachmentMacros;
};

std::shared_ptr<NodeAnimationInfo> findNodesForWithAnimation(const aiScene* scene, const std::vector<aiNode*>& usedNodes, float modelScale);

AnimationResults generateAnimationForScene(const aiScene* scene, CFileDefinition &fileDefinition, DisplayListSettings& settings);

#endif