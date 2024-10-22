#ifndef __MATERIAL_MATERIALTRANSLATOR_H__
#define __MATERIAL_MATERIALTRANSLATOR_H__

#include <assimp/scene.h>
#include "./Material.h"
#include "./TextureCache.h"
#include "../DisplayListSettings.h"

void fillMissingMaterials(TextureCache& cache, const aiScene* fromScene, DisplayListSettings& settings);

#endif