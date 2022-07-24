#ifndef __LEVEL_METADATA_H__
#define __LEVEL_METADATA_H__

#include "level_definition.h"

struct LevelMetadata {
    struct LevelDefinition* levelDefinition;
    char* segmentRomStart;
    char* segmentRomEnd;
    char* segmentStart;
};

#endif