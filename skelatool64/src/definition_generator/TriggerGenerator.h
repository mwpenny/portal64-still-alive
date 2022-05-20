#ifndef __TRIGGER_GENERATOR_H__
#define __TRIGGER_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include <map>

struct Trigger {
    std::string name;
    aiAABB bb;
};

struct TriggerGeneratorOutput {
    std::vector<std::shared_ptr<Trigger>> triggers;
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
    std::vector<std::shared_ptr<CutsceneStep>> steps;
};

class TriggerGenerator : public DefinitionGenerator {
public:
    TriggerGenerator(const DisplayListSettings& settings);

    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    const TriggerGeneratorOutput& GetOutput() const;
private:
    DisplayListSettings mSettings;
    TriggerGeneratorOutput mOutput;

    void GenerateCutscenes(std::map<std::string, std::shared_ptr<Cutscene>>& output, CFileDefinition& fileDefinition);
};

#endif