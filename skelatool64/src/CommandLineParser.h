#ifndef _COMMAND_LINE_PARSER_H
#define _COMMAND_LINE_PARSER_H

#include <assimp/mesh.h>
#include <string>
#include <vector>
#include <string.h>
#include <iostream>

enum class FileOutputType {
    Mesh,
    Level,
    Materials,
    CollisionMesh,
};

struct CommandLineArguments {
    std::string mInputFile;
    std::string mOutputFile;
    FileOutputType mOutputType;
    std::string mPrefix;
    std::vector<std::string> mMaterialFiles;
    std::string mDefaultMaterial;
    float mFixedPointScale;
    float mModelScale;
    bool mExportAnimation;
    bool mExportGeometry;
    aiVector3D mEulerAngles;
};

bool parseCommandLineArguments(int argc, char *argv[], struct CommandLineArguments& output);

#endif