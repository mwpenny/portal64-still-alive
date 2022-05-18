#ifndef _COMMAND_LINE_PARSER_H
#define _COMMAND_LINE_PARSER_H

#include <assimp/mesh.h>
#include <string>
#include <vector>
#include <string.h>
#include <iostream>

struct CommandLineArguments {
    std::string mInputFile;
    std::string mOutputFile;
    std::string mMaterialOutput;
    std::string mPrefix;
    std::vector<std::string> mMaterialFiles;
    float mGraphicsScale;
    float mCollisionScale;
    bool mExportAnimation;
    bool mExportGeometry;
    bool mIsLevel;
    bool mIsLevelDef;
    aiVector3D mEulerAngles;
};

bool parseCommandLineArguments(int argc, char *argv[], struct CommandLineArguments& output);

#endif