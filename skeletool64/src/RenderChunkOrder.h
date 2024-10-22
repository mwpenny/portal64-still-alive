#ifndef __RENDER_CHUNK_ORDER_H__
#define __RENDER_CHUNK_ORDER_H__

#include <vector>
#include "RenderChunk.h"
#include "CFileDefinition.h"
#include "DisplayListSettings.h"

struct EstimatedTime {
    EstimatedTime();
    EstimatedTime(double total, double materialSwitching, double matrixSwitching);
    
    double GetTotal() const;

    double materialSwitching;
    double matrixSwitching;
};

void orderRenderChunks(std::vector<RenderChunk>& chunks, const DisplayListSettings& settings);

#endif