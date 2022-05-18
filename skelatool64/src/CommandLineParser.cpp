
#include "CommandLineParser.h"

void parseEulerAngles(const std::string& input, aiVector3D& output) {
    std::size_t firstComma = input.find(',');
    std::size_t secondComma = input.find(',', firstComma + 1);
    output.x = (float)atof(input.substr(0, firstComma).c_str());
    output.y = (float)atof(input.substr(firstComma + 1, secondComma - (firstComma + 1)).c_str());
    output.z = (float)atof(input.substr(secondComma + 1).c_str());
}

bool parseCommandLineArguments(int argc, char *argv[], struct CommandLineArguments& output) {
    output.mInputFile = "";
    output.mOutputFile = "";
    output.mPrefix = "output";
    output.mGraphicsScale = 256.0f;
    output.mCollisionScale = 1.0f;
    output.mExportAnimation = true;
    output.mExportGeometry = true;
    output.mIsLevel = false;
    output.mIsLevelDef = false;
    output.mEulerAngles = aiVector3D(0.0f, 0.0f, 0.0f);

    char lastParameter = '\0';
    bool hasError = false;

    for (int i = 1; i < argc; ++i) {
        char* curr = argv[i];

        if (lastParameter != '\0') {
            switch (lastParameter) {
                case 'o':
                    if (output.mOutputFile != "") {
                        std::cerr << "You can only specify a single output file" << std::endl;
                        hasError = true;
                    }

                    output.mOutputFile = curr;
                    break;
                case 'n':
                    output.mPrefix = curr;
                    break;
                case 's':
                    output.mGraphicsScale = (float)atof(curr);
                    break;
                case 'c':
                    output.mCollisionScale = (float)atof(curr);
                    break;
                case 'm':
                    output.mMaterialFiles.push_back(curr);
                    break;
                case 'M':
                    output.mMaterialOutput = curr;
                    break;
                case 'r':
                    parseEulerAngles(curr, output.mEulerAngles);
                    break;
            }

            lastParameter = '\0';
        } else if (
            strcmp(curr, "-o") == 0 || 
            strcmp(curr, "--output") == 0) {
            lastParameter = 'o';
        } else if (
            strcmp(curr, "-n") == 0 || 
            strcmp(curr, "--name") == 0) {
            lastParameter = 'n';
        } else if (
            strcmp(curr, "-s") == 0 || 
            strcmp(curr, "--scale") == 0) {
            lastParameter = 's';
        } else if (
            strcmp(curr, "-c") == 0 || 
            strcmp(curr, "--collision-scale") == 0) {
            lastParameter = 'c';
        } else if (
            strcmp(curr, "-m") == 0 || 
            strcmp(curr, "--materials") == 0) {
            lastParameter = 'm';
        } else if (
            strcmp(curr, "-M") == 0 || 
            strcmp(curr, "--material-output") == 0) {
            lastParameter = 'M';
        } else if (
            strcmp(curr, "-r") == 0 || 
            strcmp(curr, "--rotate") == 0) {
            lastParameter = 'r';
        } else if (
            strcmp(curr, "-a") == 0 || 
            strcmp(curr, "--animations-only") == 0) {
            output.mExportGeometry = false;
        } else if (
            strcmp(curr, "-l") == 0 || 
            strcmp(curr, "--level") == 0) {
                    output.mIsLevel = true;
                    output.mExportAnimation = false;
        } else if (
            strcmp(curr, "-d") == 0 || 
            strcmp(curr, "--level-def") == 0) {
                    output.mIsLevelDef = true;
                    output.mExportAnimation = false;
        } else {
            if (curr[0] == '-') {
                hasError = true;
                std::cerr << "Unrecognized argument '" << curr << '"' << std::endl;
            } else if (output.mInputFile == "") {
                output.mInputFile = curr;
            } else {
                hasError = true;
                std::cerr << "Only one input file allowed. " << 
                    "Already gave '" << output.mInputFile << "'" <<
                    ". And then got '" << curr << "'" << std::endl;
            }
        }
    }

    if ((output.mInputFile == "" || (output.mOutputFile == "" && !output.mIsLevelDef)) && output.mMaterialOutput == "") {
        std::cerr << "Input and output file are both required" << std::endl;
        hasError = true;
    }

    if (hasError) {
        std::cerr << "usage " << argv[0] << " [ARGS] -o [output-file] [input-file]" << std::endl;
    }

    if (output.mOutputFile.length() > 2 && output.mOutputFile.substr(output.mOutputFile.length() - 2) == ".h") {
        output.mOutputFile = output.mOutputFile.substr(0, output.mOutputFile.length() - 2);
    }

    return !hasError;
}