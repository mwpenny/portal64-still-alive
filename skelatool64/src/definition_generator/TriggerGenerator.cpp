#include "TriggerGenerator.h"
#include "../StringUtils.h"

#define TRIGGER_PREFIX  "@trigger_"

#define CUTSCENE_PREFIX    "@cutscene "

CutsceneStep::CutsceneStep(
    const std::string& command, 
    const std::vector<std::string>& args, 
    const aiVector3D& location, 
    const aiQuaternion& direction): 
        command(command), 
        args(args),
        location(location),
        direction() {
            this->direction.Conjugate();
        }

bool doesBelongToCutscene(const CutsceneStep& startStep, const CutsceneStep& otherStep) {
    aiVector3D offset = otherStep.location - startStep.location;
    aiVector3D relativeOffset = startStep.direction.Rotate(offset);
    std::cout << relativeOffset.x << std::endl;

    return relativeOffset.y >= 0.0f && relativeOffset.x * relativeOffset.x + relativeOffset.z * relativeOffset.z < 0.1f;
}

TriggerGenerator::TriggerGenerator(const DisplayListSettings& settings): DefinitionGenerator(), mSettings(settings) {}

bool TriggerGenerator::ShouldIncludeNode(aiNode* node) {
    return (StartsWith(node->mName.C_Str(), TRIGGER_PREFIX) && node->mNumMeshes >= 1) || StartsWith(node->mName.C_Str(), CUTSCENE_PREFIX);
}

void TriggerGenerator::GenerateCutscenes(std::map<std::string, std::shared_ptr<Cutscene>>& output, CFileDefinition& fileDefinition) {
    std::vector<std::shared_ptr<CutsceneStep>> steps;

    for (auto& node : mIncludedNodes) {
        if (!StartsWith(node->mName.C_Str(), CUTSCENE_PREFIX)) {
            continue;
        }
        std::vector<std::string> cutsceneParts;
        SplitString(node->mName.C_Str(), ' ', cutsceneParts);

        if (cutsceneParts.size() <= 1) {
            continue;
        }

        std::string command = cutsceneParts[1];

        cutsceneParts.erase(cutsceneParts.begin(), cutsceneParts.begin() + 2);

        aiVector3D scale;
        aiVector3D pos;
        aiQuaternion rot;
        node->mTransformation.Decompose(scale, rot, pos);

        std::shared_ptr<CutsceneStep> step(new CutsceneStep(cutsceneParts[1], cutsceneParts, pos, rot));

        if (step->command == "start" && step->args.size() > 0) {
            std::shared_ptr<Cutscene> cutscene(new Cutscene());
            cutscene->name = step->args[0];
            cutscene->steps.push_back(step);
            output[step->args[0]] = cutscene;
        } else {
            steps.push_back(step);
        }
    }

    for (auto& cutscene : output) {
        for (auto& step : steps) {
            if (doesBelongToCutscene(*cutscene.second->steps[0], *step)) {
                cutscene.second->steps.push_back(step);
            }
        }
    }
}

void TriggerGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::map<std::string, std::shared_ptr<Cutscene>> cutscenes;

    GenerateCutscenes(cutscenes, fileDefinition);

    for (auto& node : mIncludedNodes) {
        if (!StartsWith(node->mName.C_Str(), TRIGGER_PREFIX)) {
            continue;
        }

        auto mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[0]]);

        std::shared_ptr<Trigger> trigger(new Trigger());

        std::string nodeName = node->mName.C_Str();
        trigger->name = nodeName.substr(strlen(TRIGGER_PREFIX));
        trigger->bb.mMin = node->mTransformation * mesh->bbMin * mSettings.mCollisionScale;
        trigger->bb.mMax = node->mTransformation * mesh->bbMax * mSettings.mCollisionScale;
        mOutput.triggers.push_back(trigger);
    }
}

const TriggerGeneratorOutput& TriggerGenerator::GetOutput() const {
    return mOutput;
}