#ifndef __ZSORTER_H__
#define __ZSORTER_H__

#include "./RenderChunk.h"
#include "./DisplayListSettings.h"

std::vector<RenderChunk> renderChunksSortByZ(const aiScene* scene, const std::vector<RenderChunk>& source, DisplayListSettings& settings, BoneHierarchy& boneHeirarchy);

#endif