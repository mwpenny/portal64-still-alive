#ifndef __MESH_DEFINTION_GENERATOR_H__
#define __MESH_DEFINTION_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "../DisplayListSettings.h"
#include "../RenderChunk.h"

class MeshDefinitionGenerator : public DefinitionGenerator {
public:
    MeshDefinitionGenerator(const DisplayListSettings& settings);

    virtual bool ShouldIncludeNode(aiNode* node);
    virtual void GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition);

    static void AppendRenderChunks(const aiScene* scene, aiNode* node, CFileDefinition& fileDefinition, DisplayListSettings& settings, std::vector<RenderChunk>& renderChunks);
private:
    DisplayListSettings mSettings;
};

#endif