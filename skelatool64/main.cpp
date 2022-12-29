
#include <assimp/mesh.h>

#include <iostream>
#include <fstream>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assimp/postprocess.h>

#include "src/SceneWriter.h"
#include "src/CommandLineParser.h"
#include "src/materials/MaterialParser.h"
#include "src/SceneLoader.h"
#include "src/FileUtils.h"

#include "src/definition_generator/MeshDefinitionGenerator.h"
#include "src/definition_generator/CollisionGenerator.h"
#include "src/definition_generator/MaterialGenerator.h"
#include "src/materials/MaterialState.h"
#include "src/materials/MaterialTranslator.h"
#include "src/StringUtils.h"
#include "src/lua_generator/LuaGenerator.h"

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


bool parseMaterials(const std::string& filename, DisplayListSettings& output) {
    std::fstream file(filename, std::ios::in);

    struct ParseResult parseResult(DirectoryName(filename));
    parseResult.mForcePallete = output.mForcePallete;
    parseResult.mTargetCIBuffer = output.mTargetCIBuffer;
    parseMaterialFile(file, parseResult);
    output.mMaterials.insert(parseResult.mMaterialFile.mMaterials.begin(), parseResult.mMaterialFile.mMaterials.end());

    for (auto err : parseResult.mErrors) {
        std::cerr << "Error parsing file " << filename << std::endl;
        std::cerr << err.mMessage << std::endl;
    }

    return parseResult.mErrors.size() == 0;
}

bool getVectorByName(const aiScene* scene, const std::string name, aiVector3D& result) {
    int axis;
    if (!scene->mMetaData->Get(name, axis)) {
        return false;
    }

    switch (axis) {
        case 0: result = aiVector3D(1.0f, 0.0f, 0.0f); break;
        case 1: result = aiVector3D(0.0f, 1.0f, 0.0f); break;
        case 2: result = aiVector3D(0.0f, 0.0f, 1.0f); break;
        default: return false;
    }

    int upSign;
    if (scene->mMetaData->Get(name + "Sign", upSign)) {
        if (upSign < 0) {
            result = -result;
        }
    }

    return true;
}

float angleBetween(const aiVector3D& a, const aiVector3D& b) {
    return acos(a * b / (a - b).SquareLength());
}

aiQuaternion getUpRotation(const aiVector3D& euler) {
    return aiQuaternion(euler.y * M_PI / 180.0f, euler.z * M_PI / 180.0f, euler.x * M_PI / 180.0f);
}

/**
 * F3DEX2 - 32 vertices in buffer
 * F3D - 16 vetcies in buffer
 */

int main(int argc, char *argv[]) {
    signal(SIGSEGV, handler);
    CommandLineArguments args;

    if (!parseCommandLineArguments(argc, argv, args)) {
        return 1;
    }

    DisplayListSettings settings = DisplayListSettings();

    settings.mFixedPointScale = args.mFixedPointScale;
    settings.mModelScale = args.mModelScale;
    settings.mRotateModel = getUpRotation(args.mEulerAngles);
    settings.mPrefix = args.mPrefix;
    settings.mExportAnimation = args.mExportAnimation;
    settings.mExportGeometry = args.mExportGeometry;
    settings.mBonesAsVertexGroups = args.mBonesAsVertexGroups;
    settings.mForcePallete = args.mForcePallete;
    settings.mTargetCIBuffer = args.mTargetCIBuffer;
    settings.mTicksPerSecond = args.mFPS;

    bool hasError = false;

    for (auto materialFile = args.mMaterialFiles.begin(); materialFile != args.mMaterialFiles.end(); ++materialFile) {
        if (EndsWith(*materialFile, ".yaml") || EndsWith(*materialFile, ".yml") || EndsWith(*materialFile, ".json")) {
            if (!parseMaterials(*materialFile, settings)) {
                hasError = true;
            }
        } else {
            aiScene* materialScene = loadScene(*materialFile, false, settings.mVertexCacheSize, 0);

            if (!materialScene) {
                hasError = true;
            }

            fillMissingMaterials(gTextureCache, materialScene, settings);
        }
    }

    if (hasError) {
        return 1;
    }

    auto defaultMaterial = settings.mMaterials.find(args.mDefaultMaterial);

    if (defaultMaterial != settings.mMaterials.end()) {
        settings.mDefaultMaterialState = defaultMaterial->second->mState;
        settings.mDefaultMaterialName = args.mDefaultMaterial;
        settings.mForceMaterialName = args.mForceMaterialName;
    }

    const aiScene* scene = NULL;

    unsigned int additionalPFlags = 0;

    if (settings.NeedsTangents()) {
        additionalPFlags |= aiProcess_CalcTangentSpace;
    }

    if (args.mInputFile.length()) {
        std::cout << "Generating from mesh "  << args.mInputFile << std::endl;
        scene = loadScene(args.mInputFile, args.mProcessAsModel || args.mOutputType != FileOutputType::Mesh, settings.mVertexCacheSize, additionalPFlags);

        if (!scene) {
            return 1;
        }

        fillMissingMaterials(gTextureCache, scene, settings);
    }

    std::cout << "Saving to "  << args.mOutputFile << std::endl;
    CFileDefinition fileDef(settings.mPrefix, settings.mFixedPointScale, settings.mModelScale, settings.mRotateModel);

    switch (args.mOutputType)
    {
        case FileOutputType::Mesh:
        {
            MeshDefinitionGenerator meshGenerator(settings);
            std::cout << "Generating mesh definitions" << std::endl;
            meshGenerator.TraverseScene(scene);
            meshGenerator.PopulateBones(scene, fileDef);
            meshGenerator.GenerateDefinitions(scene, fileDef);
            break;
        }
        case FileOutputType::Materials:
        {
            std::cout << "Saving materials to "  << args.mOutputFile << std::endl;

            MaterialGenerator materialGenerator(settings);

            materialGenerator.TraverseScene(scene);
            materialGenerator.GenerateDefinitions(scene, fileDef);
            break;
        }
        case FileOutputType::CollisionMesh:
        {
            NodeGroups nodesByGroup(scene);
            std::cout << "Generating collider definitions" << std::endl;
            auto collisionOutput = generateCollision(scene, fileDef, settings, nodesByGroup);
            generateMeshCollider(fileDef, *collisionOutput);
            break;
        }
        case FileOutputType::Script:
        {
            NodeGroups nodesByGroup(scene);

            for (auto script : args.mScriptFiles) {
                std::cout << "Generating definitions from script " << script << std::endl;
                generateFromLuaScript(args.mInputFile, script, scene, fileDef, nodesByGroup, settings);
            }
            break;
        }
    }

    std::cout << "Writing output" << std::endl;
    fileDef.GenerateAll(args.mOutputFile);
    
    return 0;
}