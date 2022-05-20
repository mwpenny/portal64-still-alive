#ifndef __TRIGGER_GENERATOR_H__
#define __TRIGGER_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"

struct Trigger {
    std::string name;
    aiAABB bb;
};

struct TriggerGeneratorOutput {
    std::vector<std::shared_ptr<Trigger>> triggers;
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
};

#endif