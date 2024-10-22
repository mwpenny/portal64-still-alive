#include "DefinitionGenerator.h"

#include <iostream>

#include "../StringUtils.h"

DefinitionGenerator::DefinitionGenerator() {}
DefinitionGenerator::~DefinitionGenerator() {}

void DefinitionGenerator::TraverseScene(const aiScene* scene) {
    if (!scene) {
        return;
    }

    BeforeTraversal(scene);

    forEachNode(scene->mRootNode, [&](aiNode* node) -> void { 
        if (ShouldIncludeNode(node)) {
            mIncludedNodes.push_back(node);
        }
    });
}

void DefinitionGenerator::BeforeTraversal(const aiScene* scene) {}

NodeGroups::NodeGroups(const aiScene* scene) {
    RecurseAddNode(scene->mRootNode, false);
}

void NodeGroups::RecurseAddNode(aiNode* node, bool isSpecialNode) {
    if (!node) {
        return;
    }

    if (node->mName.data[0] == '@') {
        AddNode(node);
        isSpecialNode = true;
    } else if (!isSpecialNode) {
        AddNode(node);
    }


    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        RecurseAddNode(node->mChildren[i], isSpecialNode);
    }
}

std::vector<NodeWithArguments>& NodeGroups::NodesForType(const std::string& typeName) {
    mTypesReferenced.insert(typeName);
    return mNodesByType[typeName];
}

void NodeGroups::PrintUnusedTypes() {
    for (auto& byType : mNodesByType) {
        if (mTypesReferenced.find(byType.first) == mTypesReferenced.end()) {
            std::cout << "The node type " << byType.first << " was never referenced." << std::endl;
        }
    }
}

void NodeGroups::AddNode(aiNode* node) {
    NodeWithArguments result;
    SplitString(node->mName.C_Str(), ' ', result.arguments);
    std::string typeName;
    result.node = node;

    if (result.arguments[0][0] == '@') {
        typeName = result.arguments[0];
        result.arguments.erase(result.arguments.begin());
    }

    mNodesByType[typeName].push_back(result);
}

std::string NodeWithArguments::ReadNamedArgument(const std::string& name) {
    auto parameterValue = std::find(arguments.begin(), arguments.end(), name);

    if (parameterValue == arguments.end() || parameterValue + 1 == arguments.end()) {
        return "";
    }

    return *(parameterValue + 1);

}

void forEachNode(aiNode* node, const std::function<void(aiNode*)>& callback) {
    if (!node) {
        return;
    }

    callback(node);

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        forEachNode(node->mChildren[i], callback);
    }
}