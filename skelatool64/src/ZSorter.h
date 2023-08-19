#ifndef __ZSORTER_H__
#define __ZSORTER_H__

#include "./RenderChunk.h"

std::vector<RenderChunk> renderChunksSortByZ(const std::vector<RenderChunk>& source, const aiVector3D& direction, unsigned maxBufferSize, BoneHierarchy& boneHeirarchy);

#endif