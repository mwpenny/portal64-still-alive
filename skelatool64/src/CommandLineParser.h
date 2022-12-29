#ifndef _COMMAND_LINE_PARSER_H
#define _COMMAND_LINE_PARSER_H

#include <assimp/mesh.h>
#include <string>
#include <vector>
#include <string.h>
#include <iostream>

enum class FileOutputType {
    Mesh,
    Materials,
    CollisionMesh,
    Script,
};

struct CommandLineArguments {
    std::string mInputFile;
    std::string mOutputFile;
    FileOutputType mOutputType;
    std::string mPrefix;
    std::vector<std::string> mMaterialFiles;
    std::vector<std::string> mScriptFiles;
    std::string mDefaultMaterial;
    std::string mForceMaterialName;
    std::string mForcePallete;
    float mFixedPointScale;
    float mModelScale;
    float mFPS;
    bool mExportAnimation;
    bool mExportGeometry;
    bool mBonesAsVertexGroups;
    bool mTargetCIBuffer;
    bool mProcessAsModel;
    aiVector3D mEulerAngles;
};

bool parseCommandLineArguments(int argc, char *argv[], struct CommandLineArguments& output);

#endif