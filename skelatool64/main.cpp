
#include <assimp/mesh.h>

#include <iostream>
#include <fstream>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "src/SceneWriter.h"
#include "src/CommandLineParser.h"
#include "src/materials/MaterialParser.h"
#include "src/SceneLoader.h"
#include "src/FileUtils.h"

#include "src/definition_generator/MeshDefinitionGenerator.h"
#include "src/definition_generator/CollisionGenerator.h"
#include "src/definition_generator/MaterialGenerator.h"
#include "src/definition_generator/StaticGenerator.h"
#include "src/definition_generator/LevelGenerator.h"
#include "src/materials/MaterialState.h"

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

    settings.mGraphicsScale = args.mGraphicsScale;
    settings.mCollisionScale = args.mCollisionScale;
    settings.mRotateModel = getUpRotation(args.mEulerAngles);
    settings.mPrefix = args.mPrefix;
    settings.mExportAnimation = args.mExportAnimation;
    settings.mExportGeometry = args.mExportGeometry;

    bool hasError = false;

    for (auto materialFile = args.mMaterialFiles.begin(); materialFile != args.mMaterialFiles.end(); ++materialFile) {
        if (!parseMaterials(*materialFile, settings)) {
            hasError = true;
        }
    }

    if (hasError) {
        return 1;
    }

    const aiScene* scene = NULL;

    if (args.mInputFile.length()) {
        std::cout << "Generating from mesh "  << args.mInputFile << std::endl;
        scene = loadScene(args.mInputFile, args.mIsLevel, settings.mVertexCacheSize);

        if (!scene) {
            return 1;
        }
    }

    if (scene && args.mOutputFile.length()) {    
        std::cout << "Saving to "  << args.mOutputFile << std::endl;
        // generateMeshFromSceneToFile(scene, args.mOutputFile, settings);

        MeshDefinitionGenerator meshGenerator(settings);

        std::cout << "Generating mesh definitions" << std::endl;
        meshGenerator.TraverseScene(scene);
        CFileDefinition fileDef(settings.mPrefix, settings.mGraphicsScale, settings.mRotateModel);
        meshGenerator.GenerateDefinitions(scene, fileDef);


        if (args.mIsLevel) {
            std::cout << "Generating collider definitions" << std::endl;
            CollisionGenerator colliderGenerator(settings);
            colliderGenerator.TraverseScene(scene);
            colliderGenerator.GenerateDefinitions(scene, fileDef);

            std::cout << "Generating static definitions" << std::endl;
            StaticGenerator staticGenerator(settings);
            staticGenerator.TraverseScene(scene);
            staticGenerator.GenerateDefinitions(scene, fileDef);

            std::cout << "Generating level definitions" << std::endl;
            LevelGenerator levelGenerator(settings, staticGenerator.GetOutput(), colliderGenerator.GetOutput());
            levelGenerator.GenerateDefinitions(scene, fileDef);
        }

        std::cout << "Writing output" << std::endl;
        fileDef.GenerateAll(args.mOutputFile);
    }

    if (args.mMaterialOutput.length()) {
        std::cout << "Saving materials to "  << args.mMaterialOutput << std::endl;

        MaterialGenerator materialGenerator(settings);

        materialGenerator.TraverseScene(scene);
        CFileDefinition fileDef(settings.mPrefix, settings.mGraphicsScale, settings.mRotateModel);
        materialGenerator.GenerateDefinitions(scene, fileDef);

        fileDef.GenerateAll(args.mMaterialOutput);
    }
    
    return 0;
}