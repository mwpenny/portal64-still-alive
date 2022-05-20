#include "TriggerGenerator.h"
#include "../StringUtils.h"

#define TRIGGER_PREFIX  "@trigger_"

TriggerGenerator::TriggerGenerator(const DisplayListSettings& settings): DefinitionGenerator(), mSettings(settings) {}

bool TriggerGenerator::ShouldIncludeNode(aiNode* node) {
    return StartsWith(node->mName.C_Str(), TRIGGER_PREFIX) && node->mNumMeshes >= 1;
}

void TriggerGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    for (auto& node : mIncludedNodes) {
        std::shared_ptr<Trigger> trigger(new Trigger());

        auto mesh = fileDefinition.GetExtendedMesh(scene->mMeshes[node->mMeshes[0]]);

        std::string nodeName = node->mName.C_Str();
        trigger->name = nodeName.substr(strlen(TRIGGER_PREFIX));
        trigger->bb.mMin = node->mTransformation * mesh->bbMin * mSettings.mCollisionScale;
        trigger->bb.mMax = node->mTransformation * mesh->bbMax * mSettings.mCollisionScale;
        mOutput.triggers.push_back(trigger);
    }
}

const TriggerGeneratorOutput& TriggerGenerator::GetOutput() const {
    return mOutput;
}