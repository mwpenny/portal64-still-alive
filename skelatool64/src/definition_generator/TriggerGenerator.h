#ifndef __TRIGGER_GENERATOR_H__
#define __TRIGGER_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "RoomGenerator.h"
#include "./Signals.h"
#include <map>

struct Trigger {
    std::string stepsName;
    int stepsCount;
    aiAABB bb;
};

struct Button {
    aiVector3D position;
    int roomIndex;
    int signalIndex;
};

struct TriggerGeneratorOutput {
    std::vector<std::shared_ptr<Trigger>> triggers;
    std::vector<Button> buttons;
};

struct CutsceneStep {
    CutsceneStep(const std::string& command, const std::vector<std::string>& args, const aiVector3D& location, const aiQuaternion& direction);

    std::string command;
    std::vector<std::string> args;
    aiVector3D location;
    aiQuaternion direction;
};

struct Cutscene {
    std::string name;
    std::string defName;
    std::vector<std::shared_ptr<CutsceneStep>> steps;
};

std::shared_ptr<TriggerGeneratorOutput> generateTriggers(const aiScene* scene, CFileDefinition& fileDefinition, const DisplayListSettings& settings, const RoomGeneratorOutput& roomOutput, Signals& signals, NodeGroups& nodeGroups);

void generateButtonsDefinition(CFileDefinition& fileDefinition, StructureDataChunk& levelDefinitionChunk, const std::vector<Button>& buttons);

#endif