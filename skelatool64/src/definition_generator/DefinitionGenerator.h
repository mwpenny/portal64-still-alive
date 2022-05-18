#ifndef __DEFINITION_GENERATOR_H__
#define __DEFINITION_GENERATOR_H__

#include <assimp/scene.h>

#include <vector>

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
private:
    void TraverseNodes(aiNode* node);
};

#endif