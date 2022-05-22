#ifndef __COLLISION_GENERATOR_H__
#define __COLLISION_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "CollisionQuad.h"

struct CollisionGeneratorOutput {
    std::string quadsName;
    std::vector<CollisionQuad> quads;
};

class CollisionGenerator : public DefinitionGenerator {
public:
    CollisionGenerator(const DisplayListSettings& settings);

    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    const CollisionGeneratorOutput& GetOutput() const;
private:
    DisplayListSettings mSettings;

    CollisionGeneratorOutput mOutput;
};

#endif