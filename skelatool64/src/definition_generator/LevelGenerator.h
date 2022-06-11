#ifndef __LEVEL_GENERATOR_H__
#define __LEVEL_GENERATOR_H__

#include "DefinitionGenerator.h"
#include "StaticGenerator.h"
#include "CollisionGenerator.h"
#include "../DisplayListSettings.h"
#include "TriggerGenerator.h"
#include "RoomGenerator.h"

void generateLevel(
        const aiScene* scene, 
        CFileDefinition& fileDefinition,
        const DisplayListSettings& settings,
        const StaticGeneratorOutput& staticOutput,
        const CollisionGeneratorOutput& collisionOutput,
        const TriggerGeneratorOutput& triggerOutput,
        const RoomGeneratorOutput& roomOutput,
        const SignalsOutput& signalsOutput,
        Signals& signals,
        NodeGroups& nodeGroups
);

#endif