#ifndef __ANIMATION_GENERATOR_H__
#define __ANIMATION_GENERATOR_H__

#include <assimp/scene.h>
#include <vector>
#include <memory>

#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"
#include "../Animation.h"

std::shared_ptr<NodeAnimationInfo> findNodesForWithAnimation(const aiScene* scene, const std::vector<aiNode*>& usedNodes, const aiMatrix4x4& baseTransform);

std::vector<SKAnimationHeader> generateAnimationData(const aiScene* scene, BoneHierarchy& bones, CFileDefinition& fileDef, float modelScale, unsigned short targetTicksPerSecond, aiQuaternion rotate);
void generateAnimationForScene(const aiScene* scene, CFileDefinition &fileDefinition, DisplayListSettings& settings);

#endif