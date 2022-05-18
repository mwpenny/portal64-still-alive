#ifndef _SK_ANIMATION_TRANSLATOR_H
#define _SK_ANIMATION_TRANSLATOR_H

#include <assimp/anim.h>
#include "Animation.h"
#include "BoneHierarchy.h"

bool translateAnimationToSK(const aiAnimation& input, struct SKAnimation& output, BoneHierarchy& bones, float modelScale, unsigned short targetTicksPerSecond, aiQuaternion rotate);

#endif