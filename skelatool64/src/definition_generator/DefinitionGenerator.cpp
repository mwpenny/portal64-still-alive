#include "DefinitionGenerator.h"

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


void forEachNode(aiNode* node, const std::function<void(aiNode*)>& callback) {
    if (!node) {
        return;
    }

    callback(node);

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        forEachNode(node->mChildren[i], callback);
    }
}