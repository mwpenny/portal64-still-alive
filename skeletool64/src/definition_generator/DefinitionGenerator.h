#ifndef __DEFINITION_GENERATOR_H__
#define __DEFINITION_GENERATOR_H__

#include <assimp/scene.h>

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <set>

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

struct NodeWithArguments {
    aiNode* node;
    std::vector<std::string> arguments;

    std::string ReadNamedArgument(const std::string& name);
};

class NodeGroups {
public:
    NodeGroups(const aiScene* scene);

    std::vector<NodeWithArguments>& NodesForType(const std::string& typeName);
    void PrintUnusedTypes();
private:
    void AddNode(aiNode* node);

    void RecurseAddNode(aiNode* node, bool isSpecialNode);

    std::map<std::string, std::vector<NodeWithArguments>> mNodesByType;
    std::set<std::string> mTypesReferenced;
};

void forEachNode(aiNode* node, const std::function<void(aiNode*)>& callback);

#endif