#ifndef __RENDER_CHUNK_ORDER_H__
#define __RENDER_CHUNK_ORDER_H__

#include <vector>
#include "RenderChunk.h"
#include "CFileDefinition.h"
#include "DisplayListSettings.h"

void orderRenderChunks(std::vector<RenderChunk>& chunks, const DisplayListSettings& settings);

#endif