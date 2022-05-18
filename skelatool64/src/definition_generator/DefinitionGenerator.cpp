#include "DefinitionGenerator.h"

DefinitionGenerator::DefinitionGenerator() {}
DefinitionGenerator::~DefinitionGenerator() {}

void DefinitionGenerator::TraverseScene(const aiScene* scene) {
    if (!scene) {
        return;
    }

    BeforeTraversal(scene);
    TraverseNodes(scene->mRootNode);
}

void DefinitionGenerator::BeforeTraversal(const aiScene* scene) {}

void DefinitionGenerator::TraverseNodes(aiNode* node) {
    if (!node) {
        return;
    }

    if (ShouldIncludeNode(node)) {
        mIncludedNodes.push_back(node);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        TraverseNodes(node->mChildren[i]);
    }
}