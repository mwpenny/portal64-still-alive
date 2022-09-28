
#include "AnimationTranslator.h"

#include <map>
#include <vector>
#include <climits>
#include <algorithm>
#include <iostream>

#define KEYFRAME_REMOVE_TOLERNACE   8

struct SKBoneKeyframeChain {
    SKBoneKeyframeChain();
    SKBoneKeyframe keyframe;
    unsigned short tick;
    bool isNeeded;
    int removalScore;
    struct SKBoneKeyframeChain* next;
    struct SKBoneKeyframeChain* prev;
};

SKBoneKeyframeChain::SKBoneKeyframeChain():
    tick(0),
    isNeeded(false),
    removalScore(0),
    next(nullptr),
    prev(nullptr) {

}

bool chainIsLess(SKBoneKeyframeChain* a, SKBoneKeyframeChain* b) {
    return a->removalScore > b->removalScore;
}

aiQuaternion getRotation(SKBoneKeyframe& keyframe) {
    aiQuaternion result;
    result.x = (float)keyframe.attributeData[0] / (float)std::numeric_limits<short>::max();
    result.y = (float)keyframe.attributeData[1] / (float)std::numeric_limits<short>::max();
    result.z = (float)keyframe.attributeData[2] / (float)std::numeric_limits<short>::max();

    float wsqrd = 1.0f - (result.x * result.x + result.y * result.y + result.z + result.z);
    
    if (wsqrd < 0.0f) {
        result.w = 0.0f;
    } else {
        result.w = sqrtf(wsqrd);
    }

    return result;
}

void writeRotation(const aiQuaternion& quaternion, std::vector<short>& output) {
    if (quaternion.w < 0.0f) {
        output.push_back((short)(-quaternion.x * std::numeric_limits<short>::max()));
        output.push_back((short)(-quaternion.y * std::numeric_limits<short>::max()));
        output.push_back((short)(-quaternion.z * std::numeric_limits<short>::max()));
    } else {
        output.push_back((short)(quaternion.x * std::numeric_limits<short>::max()));
        output.push_back((short)(quaternion.y * std::numeric_limits<short>::max()));
        output.push_back((short)(quaternion.z * std::numeric_limits<short>::max()));
    }
}

void guessInterpolatedValue(struct SKBoneKeyframeChain* prev, struct SKBoneKeyframeChain* next, unsigned short tick, bool isRotation, std::vector<short>& guessedValues) {
    if (!next || !prev) {
        return;
    }

    unsigned short timeOffset = next->tick - prev->tick;
    unsigned short timeToCurr = tick - prev->tick;

    if (timeOffset == 0) {
        timeOffset = 1;
        timeToCurr = 0;
    }

    if (isRotation) {
        aiQuaternion prevRotation = getRotation(prev->keyframe);
        aiQuaternion nextRotation = getRotation(next->keyframe);

        aiQuaternion interpolatedQuat;

        aiQuaternion::Interpolate(
            interpolatedQuat,
            prevRotation, 
            nextRotation, 
            (float)timeToCurr / (float)timeOffset
        );

        writeRotation(interpolatedQuat, guessedValues);
    } else {
        for (unsigned i = 0; i < prev->keyframe.attributeData.size(); ++i) {
            short valueOffset = next->keyframe.attributeData[i] - prev->keyframe.attributeData[i];
            guessedValues.push_back(prev->keyframe.attributeData[i] + valueOffset * timeToCurr / timeOffset);
        }
    }
}

bool isKeyframeNeeded(struct SKBoneKeyframeChain* prev, struct SKBoneKeyframeChain* curr, struct SKBoneKeyframeChain* next) {
    if (!next || !prev) {
        return true;
    }
    
    std::vector<short> guessedValues;
    guessInterpolatedValue(prev, next, curr->tick, (curr->keyframe.usedAttributes & SKBoneAttrMaskRotation) != 0, guessedValues);

    for (unsigned i = 0; i < curr->keyframe.attributeData.size(); ++i) {
        if (abs(guessedValues[i] - curr->keyframe.attributeData[i]) > KEYFRAME_REMOVE_TOLERNACE) {
            return true;
        }
    }

    return false;
}

int keyframeRemovalError(struct SKBoneKeyframeChain* prev, struct SKBoneKeyframeChain* curr, struct SKBoneKeyframeChain* next) {
    if (!next || !prev) {
        return std::numeric_limits<int>::max();
    }

    int result = 0;

    std::vector<short> guessedValues;
    guessInterpolatedValue(prev, next, curr->tick, (curr->keyframe.usedAttributes & SKBoneAttrMaskRotation) != 0, guessedValues);

    for (unsigned i = 0; i < curr->keyframe.attributeData.size(); ++i) {
        result += abs(guessedValues[i] - curr->keyframe.attributeData[i]);
    }

    return result;
}

unsigned short keyForKeyframe(const SKBoneKeyframe& keyframe) {
    return ((unsigned short)keyframe.boneIndex << 8) | (unsigned short)(keyframe.usedAttributes & 0x7);
}

bool keyframeSortFn(const SKBoneKeyframeChain& a, const SKBoneKeyframeChain& b) {
    if (a.tick != b.tick) {
        return a.tick < b.tick;
    }
    
    if (a.keyframe.boneIndex != b.keyframe.boneIndex) {
        return a.keyframe.boneIndex < b.keyframe.boneIndex;
    }

    return (a.keyframe.usedAttributes & 0x7) < (b.keyframe.usedAttributes & 0x7);
}

void populateKeyframes(const aiAnimation& input, BoneHierarchy& bones, float fixedPointScale, float modelScale, const aiQuaternion& rotation, std::vector<SKBoneKeyframeChain>& output) {
    for (unsigned i = 0; i < input.mNumChannels; ++i) {
        aiNodeAnim* node = input.mChannels[i];

        Bone* targetBone = bones.BoneForName(node->mNodeName.C_Str());

        if (!targetBone) {
            continue;
        }

        for (unsigned keyIndex = 0; keyIndex < node->mNumPositionKeys; ++keyIndex) {
            aiVectorKey* vectorKey = &node->mPositionKeys[keyIndex];

            SKBoneKeyframeChain keyframe;
            keyframe.tick = (unsigned short)(vectorKey->mTime);
            keyframe.next = nullptr;
            keyframe.prev = nullptr;
            keyframe.keyframe.usedAttributes = SKBoneAttrMaskPosition;
            keyframe.keyframe.boneIndex = (unsigned char)targetBone->GetIndex();

            aiVector3D origin = vectorKey->mValue;

            if (!targetBone->GetParent()) {
                aiQuaternion constRot = rotation;
                origin = constRot.Rotate(origin) * modelScale;
            }

            keyframe.keyframe.attributeData.push_back((short)(origin.x * fixedPointScale));
            keyframe.keyframe.attributeData.push_back((short)(origin.y * fixedPointScale));
            keyframe.keyframe.attributeData.push_back((short)(origin.z * fixedPointScale));
            output.push_back(keyframe);
        }

        for (unsigned keyIndex = 0; keyIndex < node->mNumRotationKeys; ++keyIndex) {
            aiQuatKey* quatKey = &node->mRotationKeys[keyIndex];

            SKBoneKeyframeChain keyframe;
            keyframe.tick = (unsigned short)(quatKey->mTime);
            keyframe.next = nullptr;
            keyframe.prev = nullptr;
            keyframe.keyframe.usedAttributes = SKBoneAttrMaskRotation;
            keyframe.keyframe.boneIndex = (unsigned char)targetBone->GetIndex();
            if (targetBone->GetParent()) {
                writeRotation(quatKey->mValue, keyframe.keyframe.attributeData);
            } else {
                writeRotation(rotation * quatKey->mValue, keyframe.keyframe.attributeData);
            }
            output.push_back(keyframe);
        }

        for (unsigned keyIndex = 0; keyIndex < node->mNumScalingKeys; ++keyIndex) {
            aiVectorKey* vectorKey = &node->mScalingKeys[keyIndex];

            SKBoneKeyframeChain keyframe;
            keyframe.tick = (unsigned short)(vectorKey->mTime);
            keyframe.next = nullptr;
            keyframe.prev = nullptr;
            keyframe.keyframe.usedAttributes = SKBoneAttrMaskScale;
            keyframe.keyframe.boneIndex = (unsigned char)targetBone->GetIndex();

            aiVector3D scale = vectorKey->mValue;

            if (!targetBone->GetParent()) {
                scale = scale * modelScale;
            }

            keyframe.keyframe.attributeData.push_back((short)(scale.x * 0x100));
            keyframe.keyframe.attributeData.push_back((short)(scale.y * 0x100));
            keyframe.keyframe.attributeData.push_back((short)(scale.z * 0x100));
            output.push_back(keyframe);
        }
    }

    std::sort(output.begin(), output.end(), keyframeSortFn);
}

void connectKeyframeChain(std::vector<SKBoneKeyframeChain>& keyframes, std::map<unsigned short, SKBoneKeyframeChain*>& firstKeyframe) {
    for (auto it = keyframes.rbegin(); it != keyframes.rend(); ++it) {
        unsigned short key = keyForKeyframe(it->keyframe);
        auto prev = firstKeyframe.find(key);

        if (prev != firstKeyframe.end()) {
            it->next = prev->second;
            prev->second->prev = &*it;
        } else {
            it->next = nullptr;
        }

        firstKeyframe[key] = &(*it);
    }

    for (auto it = firstKeyframe.begin(); it != firstKeyframe.end(); ++it) {
        it->second->prev = nullptr;
    }
}

void removeRedundantKeyframes(std::vector<SKBoneKeyframeChain>& keyframes, std::map<unsigned short, SKBoneKeyframeChain*>& firstKeyframe) {
    std::vector<SKBoneKeyframeChain*> byRemovalScore;

    for (auto it = firstKeyframe.begin(); it != firstKeyframe.end(); ++it) {
        SKBoneKeyframeChain* prev = nullptr;
        SKBoneKeyframeChain* curr = it->second;

        while (curr) {
            curr->removalScore = keyframeRemovalError(prev, curr, curr->next);
            byRemovalScore.push_back(curr);
            std::push_heap(byRemovalScore.begin(), byRemovalScore.end(), chainIsLess);
            prev = curr;
            curr = curr->next;
        }
    }

    while (byRemovalScore.size()) {
        SKBoneKeyframeChain* curr = byRemovalScore.front();
        curr->isNeeded = isKeyframeNeeded(curr->prev, curr, curr->next);

        if (!curr->isNeeded) {
            curr->next->prev = curr->prev;
            curr->prev->next = curr->next;
            curr->next = nullptr;
            curr->prev = nullptr;
        }

        std::pop_heap(byRemovalScore.begin(), byRemovalScore.end(), chainIsLess);
        byRemovalScore.pop_back();
    }

    auto currentRead = keyframes.begin();
    auto currentWrite = keyframes.begin();

    while (currentRead != keyframes.end()) {
        if (currentRead->isNeeded) {
            *currentWrite = *currentRead;
            ++currentWrite;
        }
        ++currentRead;
    }

    keyframes.resize(currentWrite - keyframes.begin());
    firstKeyframe.clear();
    connectKeyframeChain(keyframes, firstKeyframe);
}

#define CONSTANT_INTERPOLATION_THESHOLD (7888 / 2)

void markConstantKeyframes(std::map<unsigned short, SKBoneKeyframeChain*>& firstKeyframe) {
    std::set<int> jumpCutFrames;

    double thresholdCheck = 0.0f;

    for (auto it = firstKeyframe.begin(); it != firstKeyframe.end(); ++it) {
        SKBoneKeyframeChain* curr = it->second;

        while (curr && curr->next) {
            if (curr->keyframe.attributeData == curr->next->keyframe.attributeData) {
                curr->keyframe.usedAttributes |= (curr->keyframe.usedAttributes & 0x7) << 4;
            } else if (curr->keyframe.usedAttributes & SKBoneAttrMaskPosition) {
                int maxJump = 0;

                for (size_t i = 0; i < curr->keyframe.attributeData.size() && i < curr->next->keyframe.attributeData.size(); ++i) {
                    maxJump = std::max(maxJump, curr->next->keyframe.attributeData[i] - curr->keyframe.attributeData[i]);
                }

                double maxJumpDouble = (double)maxJump / (double)(curr->next->tick - curr->tick);

                if (maxJumpDouble > CONSTANT_INTERPOLATION_THESHOLD) {
                    jumpCutFrames.insert((curr->tick << 8) | curr->keyframe.boneIndex);
                } else {
                    maxJump = std::max(thresholdCheck, maxJumpDouble);
                }
            }
            curr = curr->next;
        }
    }

    for (auto it = firstKeyframe.begin(); it != firstKeyframe.end(); ++it) {
        SKBoneKeyframeChain* curr = it->second;

        while (curr && curr->next) {
            if (jumpCutFrames.find((curr->tick << 8) | curr->keyframe.boneIndex) != jumpCutFrames.end()) {
                curr->keyframe.usedAttributes |= (curr->keyframe.usedAttributes & 0x7) << 4;
            }

            curr = curr->next;
        }
    }
}

void combineChunk(std::vector<SKBoneKeyframeChain>& chunkKeyframes, struct SKAnimationChunk& output) {
    std::sort(chunkKeyframes.begin(), chunkKeyframes.end(), keyframeSortFn);

    for (auto it = chunkKeyframes.begin(); it != chunkKeyframes.end(); ++it) {
        if (!output.keyframes.size() || output.keyframes.rbegin()->tick != it->tick) {
            SKAnimationKeyframe newKeyframe;
            newKeyframe.tick = it->tick;
            output.keyframes.push_back(newKeyframe);
        }
        auto targetKeyframe = output.keyframes.rbegin();

        if (!targetKeyframe->bones.size() || targetKeyframe->bones.rbegin()->boneIndex != it->keyframe.boneIndex) {
            SKBoneKeyframe newBone;
            newBone.usedAttributes = 0;
            newBone.boneIndex = it->keyframe.boneIndex;
            targetKeyframe->bones.push_back(newBone);
        }
        auto targetBone = targetKeyframe->bones.rbegin();
        targetBone->usedAttributes |= it->keyframe.usedAttributes;
        targetBone->attributeData.insert(targetBone->attributeData.end(), it->keyframe.attributeData.begin(), it->keyframe.attributeData.end());
    }
}

unsigned buildChunk(std::vector<SKBoneKeyframeChain>& keyframes, unsigned atIndex, unsigned short currentTick, struct SKAnimationChunk& output) {
    std::vector<SKBoneKeyframeChain> nextKeyframes;

    while (atIndex < keyframes.size() && keyframes[atIndex].tick == currentTick) {
        if (keyframes[atIndex].next) {
            nextKeyframes.push_back(*keyframes[atIndex].next);
        }
        ++atIndex;
    }

    combineChunk(nextKeyframes, output);

    return atIndex;
}

void buildInitialState(std::map<unsigned short, SKBoneKeyframeChain*>& firstKeyFrame, struct SKAnimationChunk& output) {
    std::vector<SKBoneKeyframeChain> keyframes;

    for (auto it = firstKeyFrame.begin(); it != firstKeyFrame.end(); ++it) {
        SKBoneKeyframeChain modified = *(it->second);
        modified.tick = 0;
        keyframes.push_back(modified);
    }

    combineChunk(keyframes, output);
}

bool translateAnimationToSK(const aiAnimation& input, struct SKAnimation& output, BoneHierarchy& bones, float fixedPointScale, float modelScale, const aiQuaternion& rotation, unsigned short targetTicksPerSecond) {
    std::vector<SKBoneKeyframeChain> keyframes;
    populateKeyframes(input, bones, fixedPointScale, modelScale, rotation, keyframes);

    if (keyframes.size() == 0) {
        return false;
    }

    std::map<unsigned short, SKBoneKeyframeChain*> firstKeyFrame;
    connectKeyframeChain(keyframes, firstKeyFrame);
    removeRedundantKeyframes(keyframes, firstKeyFrame);
    markConstantKeyframes(firstKeyFrame);

    struct SKAnimationChunk currentChunk;
    currentChunk.nextChunkSize = 0;
    currentChunk.nextChunkTick = 0;

    unsigned currentIndex = 0;

    buildInitialState(firstKeyFrame, currentChunk);

    output.ticksPerSecond = (unsigned short)input.mTicksPerSecond;
    output.maxTicks = (unsigned short)(input.mDuration);

    while (currentIndex < keyframes.size()) {
        unsigned short tick = keyframes[currentIndex].tick;
        currentIndex = buildChunk(keyframes, currentIndex, tick, currentChunk);

        if (currentIndex < keyframes.size()) {
            currentChunk.nextChunkTick = keyframes[currentIndex].tick;
        } else {
            currentChunk.nextChunkTick = std::numeric_limits<unsigned short>::max();
        }

        if (currentChunk.keyframes.size()) {
            output.chunks.push_back(currentChunk);
        }

        currentChunk.nextChunkSize = 0;
        currentChunk.nextChunkTick = 0;
        currentChunk.keyframes.clear();
    }

    output.chunks.rbegin()->nextChunkTick = std::numeric_limits<unsigned short>::max();

    return true;
}