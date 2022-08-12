#include "TriggerGenerator.h"
#include "../StringUtils.h"
#include "../MathUtl.h"

#define TRIGGER_PREFIX  "@trigger"
#define BUTTON_PREFIX  "@button"

#define CUTSCENE_PREFIX    "@cutscene"

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

int findCutsceneIndex(const std::vector<std::shared_ptr<Cutscene>>& cutscenes, const std::string& name) {
    for (int i = 0; i < (int)cutscenes.size(); ++i) {
        if (cutscenes[i]->name == name) {
            return i;
        }
    }

    return -1;
}

std::unique_ptr<StructureDataChunk> generateCutsceneStep(CutsceneStep& step, int stepIndex, std::map<std::string, int>& labels, const std::vector<std::shared_ptr<Cutscene>>& cutscenes, const RoomGeneratorOutput& roomOutput, Signals& signals) {
    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    if ((step.command == "play_sound" || step.command == "start_sound") && step.args.size() >= 1) {
        result->AddPrimitive<const char*>(step.command == "play_sound" ? "CutsceneStepTypePlaySound" : "CutsceneStepTypeStartSound");
        std::unique_ptr<StructureDataChunk> playSound(new StructureDataChunk());

        if (StartsWith(step.args[0], "SOUNDS_")) {
            playSound->AddPrimitive(step.args[0]);
        } else {
            playSound->AddPrimitive(std::string("SOUNDS_") + step.args[0]);
        }

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
    } else if (step.command == "q_sound" && step.args.size() >= 2) {
        result->AddPrimitive<const char*>("CutsceneStepTypeQueueSound");
        std::unique_ptr<StructureDataChunk> queueSound(new StructureDataChunk());

        if (StartsWith(step.args[0], "SOUNDS_")) {
            queueSound->AddPrimitive(step.args[0]);
        } else {
            queueSound->AddPrimitive(std::string("SOUNDS_") + step.args[0]);
        }

        // channel
        queueSound->AddPrimitive(step.args[1]);

        // volume
        if (step.args.size() >= 3) {
            queueSound->AddPrimitive((int)(std::atof(step.args[2].c_str()) * 255.0f));
        } else {
            queueSound->AddPrimitive(255);
        }

        result->Add("queueSound", std::move(queueSound));

        return result;
    } else if (step.command == "wait_for_channel" && step.args.size() >= 1) {
        result->AddPrimitive<const char*>("CutsceneStepTypeWaitForChannel");
        std::unique_ptr<StructureDataChunk> waitForChannel(new StructureDataChunk());
        waitForChannel->AddPrimitive(step.args[0]);
        result->Add("waitForChannel", std::move(waitForChannel));
        return result;
    } else if (step.command == "delay" && step.args.size() >= 1) {
        result->AddPrimitive<const char*>("CutsceneStepTypeDelay");
        result->AddPrimitive("delay", std::atof(step.args[0].c_str()));
        return result;
    } else if (step.command == "open_portal" && step.args.size() >= 1) {
        short roomLocation = roomOutput.FindLocationIndex(step.args[0]);

        if (roomLocation != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeOpenPortal");
            std::unique_ptr<StructureDataChunk> openPortal(new StructureDataChunk());
            openPortal->AddPrimitive(roomLocation);
            openPortal->AddPrimitive((step.args.size() >= 2 && step.args[1] == "1") ? 1 : 0);
            result->Add("openPortal", std::move(openPortal));
            return result;
        }
    } else if ((step.command == "set_signal" || step.command == "clear_signal") && step.args.size() >= 1) {
        result->AddPrimitive<const char*>("CutsceneStepTypeSetSignal");
        std::unique_ptr<StructureDataChunk> setSignal(new StructureDataChunk());
        setSignal->AddPrimitive(signals.SignalIndexForName(step.args[0]));
        setSignal->AddPrimitive(step.command == "set_signal" ? 1 : 0);
        result->Add("setSignal", std::move(setSignal));
        return result;
    } else if (step.command == "wait_for_signal" && step.args.size() >= 1) {
        result->AddPrimitive<const char*>("CutsceneStepTypeWaitForSignal");
        std::unique_ptr<StructureDataChunk> waitforSignal(new StructureDataChunk());
        waitforSignal->AddPrimitive(signals.SignalIndexForName(step.args[0]));
        result->Add("waitForSignal", std::move(waitforSignal));
        return result;
    } else if (step.command == "teleport_player" && step.args.size() >= 2) {
        short fromLocation = roomOutput.FindLocationIndex(step.args[0]);
        short toLocation = roomOutput.FindLocationIndex(step.args[1]);

        if (fromLocation != -1 && toLocation != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeTeleportPlayer");
            std::unique_ptr<StructureDataChunk> teleportPlayer(new StructureDataChunk());
            teleportPlayer->AddPrimitive(fromLocation);
            teleportPlayer->AddPrimitive(toLocation);
            result->Add("teleportPlayer", std::move(teleportPlayer));
            return result;
        }
    } else if (step.command == "load_level" && step.args.size() >= 1) {
        short fromLocation = roomOutput.FindLocationIndex(step.args[0]);

        if (fromLocation != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeLoadLevel");
            std::unique_ptr<StructureDataChunk> loadLevel(new StructureDataChunk());
            loadLevel->AddPrimitive(fromLocation);
            // -1 means next level
            loadLevel->AddPrimitive(-1);
            result->Add("loadLevel", std::move(loadLevel));
            return result;
        }
    } else if (step.command == "goto" && step.args.size() >= 1) {
        auto gotoLabel = labels.find(step.args[0]);

        if (gotoLabel != labels.end()) {
            result->AddPrimitive<const char*>("CutsceneStepTypeGoto");
            std::unique_ptr<StructureDataChunk> gotoStep(new StructureDataChunk());
            gotoStep->AddPrimitive(gotoLabel->second - stepIndex - 1);
            result->Add("gotoStep", std::move(gotoStep));

            return result;
        }
    } else if (step.command == "start_cutscene" && step.args.size() >= 1) {
        int cutsceneIndex = findCutsceneIndex(cutscenes, step.args[0]);

        if (cutsceneIndex != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeStartCutscene");
            std::unique_ptr<StructureDataChunk> cutscene(new StructureDataChunk());
            cutscene->AddPrimitive(cutsceneIndex);
            result->Add("cutscene", std::move(cutscene));

            return result;
        }
    } else if (step.command == "stop_cutscene" && step.args.size() >= 1) {
        int cutsceneIndex = findCutsceneIndex(cutscenes, step.args[0]);

        if (cutsceneIndex != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeStopCutscene");
            std::unique_ptr<StructureDataChunk> cutscene(new StructureDataChunk());
            cutscene->AddPrimitive(cutsceneIndex);
            result->Add("cutscene", std::move(cutscene));

            return result;
        }
    } else if (step.command == "wait_for_cutscene" && step.args.size() >= 1) {
        int cutsceneIndex = findCutsceneIndex(cutscenes, step.args[0]);

        if (cutsceneIndex != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypeWaitForCutscene");
            std::unique_ptr<StructureDataChunk> cutscene(new StructureDataChunk());
            cutscene->AddPrimitive(cutsceneIndex);
            result->Add("cutscene", std::move(cutscene));

            return result;
        }
    } else if (step.command == "hide_pedestal") {
        result->AddPrimitive<const char*>("CutsceneStepTypeHidePedestal");
        return result;
    } else if (step.command == "point_pedestal" && step.args.size() >= 1) {
        short atLocation = roomOutput.FindLocationIndex(step.args[0]);

        if (atLocation != -1) {
            result->AddPrimitive<const char*>("CutsceneStepTypePointPedestal");
            std::unique_ptr<StructureDataChunk> pointPedestal(new StructureDataChunk());
            pointPedestal->AddPrimitive(atLocation);
            result->Add("pointPedestal", std::move(pointPedestal));
            return result;
        }
    }

    result->AddPrimitive<const char*>("CutsceneStepTypeNoop");
    result->AddPrimitive("noop", 0);
    
    return result;
}

void findLabelLocations(std::vector<std::shared_ptr<CutsceneStep>>& stepNodes, std::map<std::string, int>& labelLocations) {
    std::size_t currIndex = 0;

    while (currIndex < stepNodes.size()) {
        auto curr = stepNodes.begin() + currIndex;

        if ((*curr)->command == "label" && (*curr)->args.size() >= 1) {
            labelLocations[(*curr)->args[0]] = (int)currIndex;
            stepNodes.erase(curr);
        } else {
            ++currIndex;
        }
    }
}

void generateCutscenes(std::vector<std::shared_ptr<Cutscene>>& output, CFileDefinition& fileDefinition, const RoomGeneratorOutput& roomOutput, Signals& signals, NodeGroups& nodeGroups) {
    std::vector<std::shared_ptr<CutsceneStep>> steps;

    std::vector<NodeWithArguments> stepNodes = nodeGroups.NodesForType(CUTSCENE_PREFIX);

    for (auto& nodeInfo : stepNodes) {
        if (nodeInfo.arguments.size() == 0) {
            continue;
        }

        std::string command = nodeInfo.arguments[0];

        std::vector<std::string> cutsceneParts;
        cutsceneParts.assign(nodeInfo.arguments.begin() + 1, nodeInfo.arguments.end());

        aiVector3D scale;
        aiVector3D pos;
        aiQuaternion rot;
        nodeInfo.node->mTransformation.Decompose(scale, rot, pos);

        std::shared_ptr<CutsceneStep> step(new CutsceneStep(command, cutsceneParts, pos, rot));

        if (step->command == "start" && step->args.size() > 0) {
            std::shared_ptr<Cutscene> cutscene(new Cutscene());
            cutscene->name = step->args[0];
            cutscene->steps.push_back(step);
            output.push_back(cutscene);
        } else {
            steps.push_back(step);
        }
    }

    for (auto& cutscene : output) {
        for (auto& step : steps) {
            if (doesBelongToCutscene(*cutscene->steps[0], *step)) {
                cutscene->steps.push_back(step);
            }
        }
    }


    for (auto& cutsceneEntry : output) {
        Cutscene& cutscene  = *cutsceneEntry;
        std::shared_ptr<CutsceneStep> firstStep = cutscene.steps[0];

        cutscene.steps.erase(cutscene.steps.begin());

        std::sort(cutscene.steps.begin(), cutscene.steps.end(), [&](const std::shared_ptr<CutsceneStep>& a, const std::shared_ptr<CutsceneStep>& b) -> bool {
            return distanceFromStart(*firstStep, *a) < distanceFromStart(*firstStep, *b);
        });

        std::map<std::string, int> labelLocations;

        findLabelLocations(cutscene.steps, labelLocations);

        std::unique_ptr<StructureDataChunk> steps(new StructureDataChunk());

        int currStepIndex = 0;

        for (auto& step : cutscene.steps) {
            steps->Add(std::move(generateCutsceneStep(*step, currStepIndex, labelLocations, output, roomOutput, signals)));
            ++currStepIndex;
        }

        std::string stepsName = fileDefinition.GetUniqueName(cutscene.name + "_steps");
        std::unique_ptr<FileDefinition> stepsDef(new DataFileDefinition("struct CutsceneStep", stepsName, true, "_geo", std::move(steps)));
        stepsDef->AddTypeHeader("\"../build/src/audio/clips.h\"");
        fileDefinition.AddDefinition(std::move(stepsDef));
        cutscene.stepsDefName = stepsName;
    }    
}

std::shared_ptr<TriggerGeneratorOutput> generateTriggers(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, const RoomGeneratorOutput& roomOutput, Signals& signals, NodeGroups& nodeGroups) {
    std::shared_ptr<TriggerGeneratorOutput> output(new TriggerGeneratorOutput());

    generateCutscenes(output->cutscenes, fileDefinition, roomOutput, signals, nodeGroups);

    for (auto& nodeInfo : nodeGroups.NodesForType(TRIGGER_PREFIX)) {
        auto mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[nodeInfo.node->mMeshes[0]]);

        std::shared_ptr<Trigger> trigger(new Trigger());

        std::string nodeName = nodeInfo.node->mName.C_Str();

        trigger->cutsceneName = nodeInfo.arguments.size() ? nodeInfo.arguments[0] : std::string("");

        aiVector3D minTransformed = nodeInfo.node->mTransformation * mesh->bbMin * settings.mModelScale;
        aiVector3D maxTransformed = nodeInfo.node->mTransformation * mesh->bbMax * settings.mModelScale;

        trigger->bb.mMin = min(minTransformed, maxTransformed);
        trigger->bb.mMax = max(minTransformed, maxTransformed);

        output->triggers.push_back(trigger);
    }

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    for (auto& nodeInfo : nodeGroups.NodesForType(BUTTON_PREFIX)) {
        aiQuaternion rot;
        Button button;

        (baseTransform * nodeInfo.node->mTransformation).DecomposeNoScaling(rot, button.position);
        
        button.roomIndex = roomOutput.RoomForNode(nodeInfo.node);
        button.signalIndex = nodeInfo.arguments.size() ? signals.SignalIndexForName(nodeInfo.arguments[0]) : -1;
        button.cubeSignalIndex = nodeInfo.arguments.size() > 1 ? signals.SignalIndexForName(nodeInfo.arguments[1]) : -1;

        output->buttons.push_back(button);
    }

    return output;
}

void generateButtonsDefinition(CFileDefinition& fileDefinition, StructureDataChunk& levelDefinitionChunk, const std::vector<Button>& buttons) {
    std::unique_ptr<StructureDataChunk> buttonData(new StructureDataChunk());

    for (auto& ref : buttons) {
        std::unique_ptr<StructureDataChunk> singleButton(new StructureDataChunk());

        singleButton->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(ref.position)));
        singleButton->AddPrimitive(ref.roomIndex);
        singleButton->AddPrimitive(ref.signalIndex);
        singleButton->AddPrimitive(ref.cubeSignalIndex);

        buttonData->Add(std::move(singleButton));
    }

    std::string buttonsName = fileDefinition.AddDataDefinition("buttons", "struct ButtonDefinition", true, "_geo", std::move(buttonData));

    levelDefinitionChunk.AddPrimitive("buttons", buttonsName);
    levelDefinitionChunk.AddPrimitive("buttonCount", buttons.size());
}