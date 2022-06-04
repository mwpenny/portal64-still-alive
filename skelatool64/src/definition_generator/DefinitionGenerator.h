#ifndef __DEFINITION_GENERATOR_H__
#define __DEFINITION_GENERATOR_H__

#include <assimp/scene.h>

#include <vector>
#include <functional>

#include "../CFileDefinition.h"

class DefinitionGenerator {
public:
    DefinitionGenerator();
    virtual ~DefinitionGenerator();

    void TraverseScene(const aiScene* scene);

    virtual void BeforeTraversal(const aiScene* scene);

    virtual bool ShouldIncludeNode(aiNode* node) = 0;
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) = 0;
protected:
    std::vector<aiNode*> mIncludedNodes;
};

void forEachNode(aiNode* node, const std::function<void(aiNode*)>& callback);

#endif