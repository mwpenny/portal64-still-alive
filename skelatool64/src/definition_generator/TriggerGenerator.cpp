#include "TriggerGenerator.h"
#include "../StringUtils.h"
#include "../MathUtl.h"

#define TRIGGER_PREFIX  "@trigger "

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
    aiQuaternion directionCopy = startStep.direction;
    aiVector3D relativeOffset = directionCopy.Rotate(offset);

    return relativeOffset.y >= 0.0f && relativeOffset.x * relativeOffset.x + relativeOffset.z * relativeOffset.z < 0.1f;
}

float distanceFromStart(const CutsceneStep& startStep, const CutsceneStep& otherStep) {
    aiVector3D offset = otherStep.location - startStep.location;
    aiQuaternion directionCopy = startStep.direction;
    aiVector3D relativeOffset = directionCopy.Rotate(offset);
    return relativeOffset.y;
}

TriggerGenerator::TriggerGenerator(const DisplayListSettings& settings, const RoomGeneratorOutput& roomOutput): DefinitionGenerator(), mSettings(settings), mRoomOutput(roomOutput) {}

bool TriggerGenerator::ShouldIncludeNode(aiNode* node) {
    return (StartsWith(node->mName.C_Str(), TRIGGER_PREFIX) && node->mNumMeshes >= 1) || StartsWith(node->mName.C_Str(), CUTSCENE_PREFIX);
}

std::unique_ptr<StructureDataChunk> TriggerGenerator::GenerateCutsceneStep(CutsceneStep& step) {
    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    if ((step.command == "play_sound" || step.command == "start_sound") && step.args.size() >= 1) {
        result->AddPrimitive<const char*>(step.command == "play_sound" ? "CutsceneStepTypePlaySound" : "CutsceneStepTypeStartSound");
        std::unique_ptr<StructureDataChunk> playSound(new StructureDataChunk());
        playSound->AddPrimitive(step.args[0]);

        if (step.args.size() >= 2) {
            playSound->AddPrimitive((int)(std::atof(step.args[1].c_str()) * 255.0f));
        } else {
            playSound->AddPrimitive(255);
        }

        if (step.args.size() >= 3) {
            playSound->AddPrimitive((int)(std::atof(step.args[2].c_str()) * 64.0f + 0.5f));
        } else {
            playSound->AddPrimitive(64);
        }

        result->Add("playSound", std::move(playSound));

        return result;
    } else if (step.command == "delay" && step.args.size() >= 1) {
        result->AddPrimitive<const char*>("CutsceneStepTypeDelay");
        result->AddPrimitive("delay", std::atof(step.args[0].c_str()));
        return result;
    } else if (step.command == "open_portal" && step.args.size() >= 1) {
        short roomLocation = mRoomOutput.FindLocationIndex(step.args[0]);

        if (roomLocation != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeOpenPortal");
            std::unique_ptr<StructureDataChunk> openPortal(new StructureDataChunk());
            openPortal->AddPrimitive(roomLocation);
            openPortal->AddPrimitive((step.args.size() >= 2 && step.args[1] == "1") ? 1 : 0);
            result->Add("openPortal", std::move(openPortal));
            return result;
        }
    }

    result->AddPrimitive<const char*>("CutsceneStepTypeNoop");
    result->AddPrimitive("noop", 0);
    
    return result;
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

        std::shared_ptr<CutsceneStep> step(new CutsceneStep(command, cutsceneParts, pos, rot));

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


    for (auto& cutsceneEntry : output) {
        Cutscene& cutscene  = *cutsceneEntry.second;
        std::shared_ptr<CutsceneStep> firstStep = cutscene.steps[0];

        cutscene.steps.erase(cutscene.steps.begin());

        std::sort(cutscene.steps.begin(), cutscene.steps.end(), [&](const std::shared_ptr<CutsceneStep>& a, const std::shared_ptr<CutsceneStep>& b) -> bool {
            return distanceFromStart(*firstStep, *a) < distanceFromStart(*firstStep, *b);
        });

        std::unique_ptr<StructureDataChunk> steps(new StructureDataChunk());

        for (auto& step : cutscene.steps) {
            steps->Add(std::move(GenerateCutsceneStep(*step)));
        }

        std::string stepsName = fileDefinition.GetUniqueName(cutscene.name + "_steps");
        std::unique_ptr<FileDefinition> stepsDef(new DataFileDefinition("struct CutsceneStep", stepsName, true, "_geo", std::move(steps)));
        stepsDef->AddTypeHeader("\"../build/src/audio/clips.h\"");
        fileDefinition.AddDefinition(std::move(stepsDef));
        cutscene.defName = stepsName;
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

        auto cutscene = cutscenes.find(nodeName.substr(strlen(TRIGGER_PREFIX)));

        if (cutscene == cutscenes.end()) {
            trigger->stepsName = "";
            trigger->stepsCount = 0;
        } else {
            trigger->stepsName = cutscene->second->defName;
            trigger->stepsCount = cutscene->second->steps.size();
        }

        aiVector3D minTransformed = node->mTransformation * mesh->bbMin * mSettings.mCollisionScale;
        aiVector3D maxTransformed = node->mTransformation * mesh->bbMax * mSettings.mCollisionScale;

        trigger->bb.mMin = min(minTransformed, maxTransformed);
        trigger->bb.mMax = max(minTransformed, maxTransformed);

        mOutput.triggers.push_back(trigger);
    }
}

const TriggerGeneratorOutput& TriggerGenerator::GetOutput() const {
    return mOutput;
}