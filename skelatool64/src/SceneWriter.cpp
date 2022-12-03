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
#include "MeshWriter.h"
#include "FileUtils.h"
#include "./definition_generator/AnimationGenerator.h"

void generateMeshFromScene(const aiScene* scene, CFileDefinition& fileDefinition, DisplayListSettings& settings) {
    BoneHierarchy bones;
    bool shouldExportAnimations;

    if (settings.mExportAnimation) {
        bones.SearchForBonesInScene(scene, settings.mFixedPointScale);
        shouldExportAnimations = bones.HasData();
    } else {
        shouldExportAnimations = false;
    }

    std::vector<std::shared_ptr<ExtendedMesh>> extendedMeshes;

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        extendedMeshes.push_back(std::shared_ptr<ExtendedMesh>(new ExtendedMesh(scene->mMeshes[i], bones)));
    }

    std::vector<RenderChunk> renderChunks;

    extractChunks(scene, extendedMeshes, renderChunks, settings.mMaterials, settings.mForceMaterialName);
    orderChunks(renderChunks);

    std::string renderDLName;

    if (settings.mExportGeometry) {
        renderDLName = generateMesh(scene, fileDefinition, renderChunks, settings, "");
    }

    if (shouldExportAnimations) {        
        generateAnimationForScene(scene, fileDefinition, settings);
    }
}

void generateMeshFromSceneToFile(const aiScene* scene, std::string filename, DisplayListSettings& settings) {
    CFileDefinition fileDefinition(settings.mPrefix, settings.mFixedPointScale, settings.mModelScale, settings.mRotateModel);

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