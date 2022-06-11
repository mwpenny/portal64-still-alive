#ifndef __DECOR_GENERATOR_H__
#define __DECOR_GENERATOR_H__

#include <assimp/scene.h>
#include "../CFileDefinition.h"
#include "RoomGenerator.h"

void generateDecorDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const RoomGeneratorOutput& roomOutput, const DisplayListSettings& settings, NodeGroups& nodeGroups);

#endif