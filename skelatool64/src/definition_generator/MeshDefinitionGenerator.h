#ifndef __MESH_DEFINTION_GENERATOR_H__
#define __MESH_DEFINTION_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "../RenderChunk.h"

struct MeshDefinitionResults {
public:
    std::string modelName;
    std::string materialMacro;
};

class MeshDefinitionGenerator : public DefinitionGenerator {
public:
    MeshDefinitionGenerator(const DisplayListSettings& settings);

    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    void PopulateBones(const aiScene* scene, CFileDefinition& fileDefinition);

    MeshDefinitionResults GenerateDefinitionsWithResults(const aiScene* scene, CFileDefinition& fileDefinition);

    static void AppendRenderChunks(const aiScene* scene, aiNode* node, CFileDefinition& fileDefinition, const DisplayListSettings& settings, std::vector<RenderChunk>& renderChunks);
private:
    DisplayListSettings mSettings;
};

#endif