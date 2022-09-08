
#include "CommandLineParser.h"

void parseEulerAngles(const std::string& input, aiVector3D& output) {
    std::size_t firstComma = input.find(',');
    std::size_t secondComma = input.find(',', firstComma + 1);
    output.x = (float)atof(input.substr(0, firstComma).c_str());
    output.y = (float)atof(input.substr(firstComma + 1, secondComma - (firstComma + 1)).c_str());
    output.z = (float)atof(input.substr(secondComma + 1).c_str());
}

bool needsInput(FileOutputType type) {
    return type != FileOutputType::Materials;
}

bool parseCommandLineArguments(int argc, char *argv[], struct CommandLineArguments& output) {
    output.mInputFile = "";
    output.mOutputFile = "";
    output.mPrefix = "output";
    output.mFixedPointScale = 256.0f;
    output.mModelScale = 1.0f;
    output.mExportAnimation = true;
    output.mExportGeometry = true;
    output.mBonesAsVertexGroups = false;
    output.mTargetCIBuffer = false;
    output.mOutputType = FileOutputType::Mesh;
    output.mEulerAngles = aiVector3D(0.0f, 0.0f, 0.0f);
    output.mDefaultMaterial = "default";
    output.mForceMaterialName = "";

    std::string lastParameter = "";
    bool hasError = false;

    for (int i = 1; i < argc; ++i) {
        char* curr = argv[i];

        if (lastParameter != "") {
            if (lastParameter == "output") {
                if (output.mOutputFile != "") {
                    std::cerr << "You can only specify a single output file" << std::endl;
                    hasError = true;
                }

                output.mOutputFile = curr;
            } else if (lastParameter == "name") {
                output.mPrefix = curr;
            } else if (lastParameter == "fixed-point-scale") {
                output.mFixedPointScale = (float)atof(curr);
            } else if (lastParameter == "model-scale") {
                output.mModelScale = (float)atof(curr);
            } else if (lastParameter == "materials") {
                output.mMaterialFiles.push_back(curr);
            } else if (lastParameter == "rotate") {
                parseEulerAngles(curr, output.mEulerAngles);
            } else if (lastParameter == "default-material") {
                output.mDefaultMaterial = curr;
            } else if (lastParameter == "force-material") {
                output.mForceMaterialName = curr;
            } else if (lastParameter == "pallete") {
                output.mForcePallete = curr;
            } else if (lastParameter == "script") {
                output.mScriptFiles.push_back(curr);
            }

            lastParameter = "";
        } else if (
            strcmp(curr, "-o") == 0 || 
            strcmp(curr, "--output") == 0) {
            lastParameter = "output";
        } else if (
            strcmp(curr, "-n") == 0 || 
            strcmp(curr, "--name") == 0) {
            lastParameter = "name";
        } else if (strcmp(curr, "--fixed-point-scale") == 0) {
            lastParameter = "fixed-point-scale";
        } else if (strcmp(curr, "--model-scale") == 0) {
            lastParameter = "model-scale";
        } else if (
            strcmp(curr, "-m") == 0 || 
            strcmp(curr, "--materials") == 0) {
            lastParameter = "materials";
        } else if (strcmp(curr, "--material-output") == 0) {
            output.mOutputType = FileOutputType::Materials;
        } else if (strcmp(curr, "--mesh-collider") == 0) {
            output.mOutputType = FileOutputType::CollisionMesh;
        } else if (
            strcmp(curr, "-r") == 0 || 
            strcmp(curr, "--rotate") == 0) {
            lastParameter = "rotate";
        } else if (
            strcmp(curr, "--pallete") == 0) {
            lastParameter = "pallete";
        } else if (
            strcmp(curr, "-a") == 0 || 
            strcmp(curr, "--animations-only") == 0) {
            output.mExportGeometry = false;
        } else if (strcmp(curr, "--boneless") == 0) {
            output.mBonesAsVertexGroups = true;
        } else if (strcmp(curr, "--level") == 0) {
                    output.mOutputType = FileOutputType::Level;
                    output.mExportAnimation = false;
        } else if (strcmp(curr, "--default-material") == 0) {
            lastParameter = "default-material";
        } else if (strcmp(curr, "--force-material") == 0) {
            lastParameter = "force-material";
        } else if (strcmp(curr, "--script") == 0) {
            output.mOutputType = FileOutputType::Script;
            lastParameter = "script";
        } else if (strcmp(curr, "--ci-buffer") == 0) {
            output.mTargetCIBuffer = true;
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

    if (output.mOutputFile == "") {
        std::cerr << "No output file specified" << std::endl;
        hasError = true;
    }

    if (output.mInputFile == "" && needsInput(output.mOutputType)) {
        std::cerr << "No input file specified" << std::endl;
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