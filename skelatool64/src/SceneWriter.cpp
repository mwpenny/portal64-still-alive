#include "SceneWriter.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <string>

#include "./DisplayList.h"
#include "./DisplayListGenerator.h"
#include "./BoneHierarchy.h"
#include "./ExtendedMesh.h"
#include "./RenderChunk.h"
#include "AnimationTranslator.h"
#include "MeshWriter.h"
#include "FileUtils.h"

std::vector<SKAnimationHeader> generateAnimationData(const aiScene* scene, BoneHierarchy& bones, CFileDefinition& fileDef, float modelScale, unsigned short targetTicksPerSecond, aiQuaternion rotate) {
    std::vector<SKAnimationHeader> animations;

    for (unsigned i = 0; i < scene->mNumAnimations; ++i) {
        SKAnimation animation;
        if (translateAnimationToSK(*scene->mAnimations[i], animation, bones, modelScale, targetTicksPerSecond, rotate)) {
            std::string animationName = fileDef.GetUniqueName(scene->mAnimations[i]->mName.C_Str());
            unsigned short firstChunkSize = formatAnimationChunks(animationName, animation.chunks, fileDef);

            SKAnimationHeader header;
            header.firstChunkSize = firstChunkSize;
            header.ticksPerSecond = targetTicksPerSecond;
            header.maxTicks = animation.maxTicks;
            header.animationName = animationName;

            animations.push_back(header);
        }
    }

    return animations;
}

void generateMeshFromScene(const aiScene* scene, CFileDefinition& fileDefinition, DisplayListSettings& settings) {
    BoneHierarchy bones;
    bool shouldExportAnimations;

    if (settings.mExportAnimation) {
        bones.SearchForBonesInScene(scene);
        shouldExportAnimations = bones.HasData();
    } else {
        shouldExportAnimations = false;
    }

    std::vector<std::shared_ptr<ExtendedMesh>> extendedMeshes;

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        extendedMeshes.push_back(std::shared_ptr<ExtendedMesh>(new ExtendedMesh(scene->mMeshes[i], bones)));
    }

    std::vector<RenderChunk> renderChunks;

    extractChunks(scene, extendedMeshes, renderChunks, settings.mMaterials);
    orderChunks(renderChunks);

    std::string renderDLName;

    if (settings.mExportGeometry) {
        renderDLName = generateMesh(scene, fileDefinition, renderChunks, settings, "");
    }

    if (shouldExportAnimations) {        
        std::string bonesName = fileDefinition.GetUniqueName("default_bones");
        std::string boneParentName = fileDefinition.GetUniqueName("bone_parent");
        bones.GenerateRestPosiitonData(fileDefinition, bonesName, settings.mGraphicsScale, settings.mRotateModel);
        std::string boneCountName = bonesName + "_COUNT";
        std::transform(boneCountName.begin(), boneCountName.end(), boneCountName.begin(), ::toupper);
        fileDefinition.AddMacro(boneCountName, std::to_string(bones.GetBoneCount()));

        std::string animationsName = fileDefinition.GetUniqueName("animations");
        auto animations = generateAnimationData(scene, bones, fileDefinition, settings.mGraphicsScale, settings.mTicksPerSecond, settings.mRotateModel);

        std::unique_ptr<StructureDataChunk> animationNameData(new StructureDataChunk());

        int index = 0;
        for (auto it = animations.begin(); it != animations.end(); ++it) {
            std::unique_ptr<StructureDataChunk> animationChunk(new StructureDataChunk());

            animationChunk->AddPrimitive(it->firstChunkSize);
            animationChunk->AddPrimitive(it->ticksPerSecond);
            animationChunk->AddPrimitive(it->maxTicks);
            animationChunk->AddPrimitive(0);
            animationChunk->AddPrimitive(std::string("(struct SKAnimationChunk*)") + it->animationName);
            animationChunk->AddPrimitive(0);

            animationNameData->Add(std::move(animationChunk));

            std::string animationIndex = fileDefinition.GetUniqueName(it->animationName + "_INDEX");
            std::transform(animationIndex.begin(), animationIndex.end(), animationIndex.begin(), ::toupper);
            fileDefinition.AddMacro(animationIndex, std::to_string(index));

            ++index;
        }
        fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct SKAnimationHeader", animationsName, true, "_anim", std::move(animationNameData))));

        std::unique_ptr<StructureDataChunk> boneParentDataChunk(new StructureDataChunk());

        for (unsigned int boneIndex = 0; boneIndex < bones.GetBoneCount(); ++boneIndex) {
            Bone* bone = bones.BoneByIndex(boneIndex);
            if (bone->GetParent()) {
                boneParentDataChunk->AddPrimitive(bone->GetParent()->GetIndex());
            } else {
                boneParentDataChunk->AddPrimitive(0xFFFF);
            }
        }

        fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("unsigned short", boneParentName, true, "_anim", std::move(boneParentDataChunk))));
    }
}

void generateMeshFromSceneToFile(const aiScene* scene, std::string filename, DisplayListSettings& settings) {
    CFileDefinition fileDefinition(settings.mPrefix, settings.mGraphicsScale, settings.mRotateModel);

    generateMeshFromScene(scene, fileDefinition, settings);

    std::string filenameBase = replaceExtension(getBaseName(filename), "");

    if (settings.mExportGeometry) {
        std::ofstream outputFile;
        outputFile.open(filename + "_geo.inc.h", std::ios_base::out | std::ios_base::trunc);
        fileDefinition.Generate(outputFile, "", filenameBase + ".h");
        outputFile.close();
    }

    std::ofstream outputHeader;
    outputHeader.open(filename + ".h", std::ios_base::out | std::ios_base::trunc);
    fileDefinition.GenerateHeader(outputHeader, filenameBase);
    outputHeader.close();

    if (fileDefinition.HasDefinitions("_anim")) {
        std::ofstream animOutput;
        animOutput.open(filename + "_anim.inc.h", std::ios_base::out | std::ios_base::trunc);
        fileDefinition.Generate(animOutput, "_anim", filenameBase + ".h");
        animOutput.close();
    }
}