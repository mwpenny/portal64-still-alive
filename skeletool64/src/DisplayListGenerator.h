#ifndef _DISPLAY_LIST_GENERATOR_H
#define _DISPLAY_LIST_GENERATOR_H

#include <assimp/mesh.h>
#include "./RCPState.h"
#include "./DisplayList.h"
#include "./CFileDefinition.h"
#include "./RenderChunk.h"

void generateCulling(DisplayList& output, std::string vertexBuffer, bool renableLighting);
void generateGeometry(RenderChunk& mesh, RCPState& state, std::string vertexBuffer, DisplayList& output, bool hasTri2);

#endif