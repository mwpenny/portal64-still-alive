#include "AnimationGenerator.h"

#include "./DefinitionGenerator.h"
#include <set>
#include <string>
#include <map>
#include "../StringUtils.h"

std::shared_ptr<NodeAnimationInfo> findNodesForWithAnimation(const aiScene* scene, const std::vector<aiNode*>& usedNodes, float modelScale) {
    std::set<std::string> animatedNodeNames;

    for (unsigned animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex) {
        aiAnimation* animation = scene->mAnimations[animIndex];

        for (unsigned channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex) {
            animatedNodeNames.insert(animation->mChannels[channelIndex]->mNodeName.C_Str());
        }
    }

    std::set<aiMesh*> usedMeshes;

    for (auto node : usedNodes) {
        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            usedMeshes.insert(scene->mMeshes[node->mMeshes[i]]);
        }
    }

    for (auto mesh : usedMeshes) {
        for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            animatedNodeNames.insert(mesh->mBones[boneIndex]->mName.C_Str());
        }
    }

    std::map<const aiNode*, int> nodeOrder;
    std::set<aiNode*> nodesWithAnimationData;

    forEachNode(scene->mRootNode, [&](aiNode* node) -> void {
        if (animatedNodeNames.find(node->mName.C_Str()) != animatedNodeNames.end()) {
            nodesWithAnimationData.insert(node);
        }

        nodeOrder.insert(std::make_pair(node, nodeOrder.size()));
    });

    std::shared_ptr<NodeAnimationInfo> result(new NodeAnimationInfo());

    for (auto node : nodesWithAnimationData) {
        std::unique_ptr<AnimationNodeInfo> nodeInfo(new AnimationNodeInfo());

        aiNode* currentNode = node;

        while (currentNode->mParent && nodesWithAnimationData.find(currentNode->mParent) == nodesWithAnimationData.end()) {
            currentNode = currentNode->mParent;

            nodeInfo->relativeTransform = currentNode->mTransformation * nodeInfo->relativeTransform;
        }

        if (!currentNode->mParent) {
            nodeInfo->relativeTransform = aiMatrix4x4(aiVector3D(1, 1, 1) * modelScale, aiQuaternion(), aiVector3D()) * nodeInfo->relativeTransform;
        }

        nodeInfo->node = node;
        nodeInfo->parent = currentNode->mParent;

        result->nodesWithAnimation.push_back(std::move(nodeInfo));
    }

    std::sort(result->nodesWithAnimation.begin(), result->nodesWithAnimation.end(), [&](const std::unique_ptr<AnimationNodeInfo>& a, const std::unique_ptr<AnimationNodeInfo>& b) -> bool {
        return nodeOrder[a->node] < nodeOrder[b->node];
    });

    return result;
}

struct FrameData {
    aiVector3D position;
    aiQuaternion rotation;
};

template <typename T>
void findStartValue(const T* keys, unsigned keyCount, double at, unsigned& startValue, double& lerp) {
    lerp = 0.0f;

    for (startValue = 0; startValue < keyCount; ++startValue) {
        if (keys[startValue].mTime == at) {
            lerp = 0.0f;
            break;
        } else if (keys[startValue].mTime > at) {
            if (startValue == 0) {
                lerp = 0.0f;
            } else {
                --startValue;
                double deltaTime = keys[startValue + 1].mTime - keys[startValue].mTime;

                if (deltaTime == 0.0) {
                    lerp = 0.0f;
                } else {
                    lerp = (at - keys[startValue].mTime) / deltaTime;
                }
            }

            break;
        }
    }
}

aiVector3D evaluateVectorAt(const aiVectorKey* keys, unsigned keyCount, double at) {
    if (keyCount == 0) {
        return aiVector3D();
    }

    if (keyCount == 1) {
        return keys[0].mValue;
    }

    unsigned startValue;
    double lerp = 0.0f;

    findStartValue(keys, keyCount, at, startValue, lerp);
 
    if (startValue == keyCount) {
        return keys[keyCount - 1].mValue;
    }

    aiVector3D from = keys[startValue].mValue;
    aiVector3D to = keys[startValue + 1].mValue;

    return (to - from) * (float)lerp + from;
}

aiQuaternion evaluateQuaternionAt(const aiQuatKey* keys, unsigned keyCount, double at) {
    if (keyCount == 0) {
        return aiQuaternion();
    }

    if (keyCount == 1) {
        return keys[0].mValue;
    }

    unsigned startValue;
    double lerp = 0.0f;
    
    findStartValue(keys, keyCount, at, startValue, lerp);
 
    if (startValue == keyCount) {
        return keys[keyCount - 1].mValue;
    }

    aiQuaternion from = keys[startValue].mValue;
    aiQuaternion to = keys[startValue + 1].mValue;
    aiQuaternion output;

    aiQuaternion::Interpolate(output, from, to, lerp);
    
    return output;
}

std::string generateanimationV2(const aiAnimation& animation, int index, BoneHierarchy& bones, CFileDefinition& fileDef, const DisplayListSettings& settings) {
    int nFrames = ceil(animation.mDuration * settings.mTicksPerSecond / animation.mTicksPerSecond) + 1;

    std::vector<std::vector<FrameData>> allFrameData(nFrames);

    for (int i = 0; i < nFrames; ++i) {
        allFrameData[i].resize(bones.GetBoneCount());
    }

    for (unsigned boneIndex = 0; boneIndex < bones.GetBoneCount(); ++boneIndex) {
        Bone* bone = bones.BoneByIndex(boneIndex);

        aiNodeAnim* nodeAnim = nullptr;

        // find the animation channel for the given frame
        for (unsigned channelIndex = 0; channelIndex < animation.mNumChannels; ++channelIndex) {
            if (bone->GetName() == animation.mChannels[channelIndex]->mNodeName.C_Str()) {
                nodeAnim = animation.mChannels[channelIndex];
                break;
            }
        }

        if (!nodeAnim) {
            continue;
        }

        // populate the frame data for the given channel
        for (int frame = 0; frame < nFrames; ++frame) {
            double at = frame * animation.mTicksPerSecond / settings.mTicksPerSecond;

            aiVector3D origin = evaluateVectorAt(nodeAnim->mPositionKeys, nodeAnim->mNumPositionKeys, at);
            aiQuaternion rotation = evaluateQuaternionAt(nodeAnim->mRotationKeys, nodeAnim->mNumRotationKeys, at);

            if (!bone->GetParent()) {
                aiQuaternion constRot = settings.mRotateModel;
                origin = constRot.Rotate(origin) * settings.mModelScale;
                rotation = constRot * rotation;
            }

            allFrameData[frame][boneIndex].position = origin * settings.mFixedPointScale;
            allFrameData[frame][boneIndex].rotation = rotation;

        }
    }

    std::unique_ptr<StructureDataChunk> frames(new StructureDataChunk());

    for (int frame = 0; frame < nFrames; ++frame) {
        for (auto& frameBone : allFrameData[frame]) {
            std::unique_ptr<StructureDataChunk> posData(new StructureDataChunk());
            std::unique_ptr<StructureDataChunk> rotData(new StructureDataChunk());

            posData->AddPrimitive((short)(frameBone.position.x));
            posData->AddPrimitive((short)(frameBone.position.y));
            posData->AddPrimitive((short)(frameBone.position.z));

            if (frameBone.rotation.w < 0.0f) {
                rotData->AddPrimitive((short)(-frameBone.rotation.x * std::numeric_limits<short>::max()));
                rotData->AddPrimitive((short)(-frameBone.rotation.y * std::numeric_limits<short>::max()));
                rotData->AddPrimitive((short)(-frameBone.rotation.z * std::numeric_limits<short>::max()));
            } else {
                rotData->AddPrimitive((short)(frameBone.rotation.x * std::numeric_limits<short>::max()));
                rotData->AddPrimitive((short)(frameBone.rotation.y * std::numeric_limits<short>::max()));
                rotData->AddPrimitive((short)(frameBone.rotation.z * std::numeric_limits<short>::max()));
            }

            std::unique_ptr<StructureDataChunk> frameData(new StructureDataChunk());
            frameData->Add(std::move(posData));
            frameData->Add(std::move(rotData));
            frames->Add(std::move(frameData));
        }
    }

    std::string framesName = fileDef.AddDataDefinition(std::string(animation.mName.C_Str()) + "_data", "struct SKAnimationBoneFrame", true, "_anim", std::move(frames));

    std::unique_ptr<StructureDataChunk> clip(new StructureDataChunk());
    clip->AddPrimitive(nFrames);
    clip->AddPrimitive(bones.GetBoneCount());
    clip->AddPrimitive(framesName);
    clip->AddPrimitive(settings.mTicksPerSecond);
    std::string result = fileDef.AddDataDefinition(std::string(animation.mName.C_Str()) + "_clip", "struct SKAnimationClip", false, "_geo", std::move(clip));

    std::string animationMacroName = fileDef.GetUniqueName(std::string(animation.mName.C_Str()) + "_clip_index");
    std::transform(animationMacroName.begin(), animationMacroName.end(), animationMacroName.begin(), ::toupper);
    fileDef.AddMacro(animationMacroName, std::to_string(index));

    return result;
}

void generateAnimationDataV2(const aiScene* scene, BoneHierarchy& bones, CFileDefinition& fileDef, const DisplayListSettings& settings) {
    std::unique_ptr<StructureDataChunk> clipArray(new StructureDataChunk());

    for (unsigned animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
        std::string clipName = generateanimationV2(*scene->mAnimations[animationIndex], animationIndex, bones, fileDef, settings);
        clipArray->AddPrimitive("&" + clipName);
    }

    fileDef.AddDataDefinition("clips", "struct SKAnimationClip*", true, "_geo", std::move(clipArray));

    std::string clipCountMacroName = fileDef.GetUniqueName("CLIP_COUNT");
    std::transform(clipCountMacroName.begin(), clipCountMacroName.end(), clipCountMacroName.begin(), ::toupper);
    fileDef.AddMacro(clipCountMacroName, std::to_string(scene->mNumAnimations));
}

AnimationResults generateAnimationForScene(const aiScene* scene, CFileDefinition &fileDefinition, DisplayListSettings& settings) {
    AnimationResults result;

    BoneHierarchy& bones = fileDefinition.GetBoneHierarchy();

    std::string bonesName = fileDefinition.GetUniqueName("default_bones");
    std::string boneParentName = fileDefinition.GetUniqueName("bone_parent");
    bones.GenerateRestPosiitonData(fileDefinition, bonesName);
    std::string boneCountName = bonesName + "_COUNT";
    std::transform(boneCountName.begin(), boneCountName.end(), boneCountName.begin(), ::toupper);
    fileDefinition.AddMacro(boneCountName, std::to_string(bones.GetBoneCount()));

    result.initialPoseReference = bonesName;
    result.boneParentReference = boneParentName;
    result.boneCountMacro = boneCountName;

    aiMatrix4x4 baseTransform(
        aiVector3D(settings.mModelScale, settings.mModelScale, settings.mModelScale), 
        settings.mRotateModel, 
        aiVector3D(0, 0, 0)
    );

    fileDefinition.AddHeader("\"sk64/skelatool_clip.h\"");

    std::unique_ptr<StructureDataChunk> boneParentDataChunk(new StructureDataChunk());

    for (unsigned int boneIndex = 0; boneIndex < bones.GetBoneCount(); ++boneIndex) {
        Bone* bone = bones.BoneByIndex(boneIndex);
        if (bone->GetParent()) {
            boneParentDataChunk->AddPrimitive(bone->GetParent()->GetIndex());
        } else {
            boneParentDataChunk->AddPrimitive(0xFFFF);
        }
    }

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("unsigned short", boneParentName, true, "_geo", std::move(boneParentDataChunk))));

    int attachmentCount = 0;

    for (unsigned i = 0; i < bones.GetBoneCount(); ++i) {
        Bone* bone = bones.BoneByIndex(i);

        if (StartsWith(bone->GetName(), "attachment ")) {
            fileDefinition.AddMacro(fileDefinition.GetMacroName(std::string("ATTACHMENT_") + bone->GetName().substr(strlen("attachment "))), std::to_string(attachmentCount));
            ++attachmentCount;
        }
    }

    result.numberOfAttachmentMacros = fileDefinition.GetMacroName("ATTACHMENT_COUNT");

    fileDefinition.AddMacro(result.numberOfAttachmentMacros, std::to_string(attachmentCount));

    generateAnimationDataV2(scene, bones, fileDefinition, settings);

    return result;
}