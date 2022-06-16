#ifndef __DECOR_OBJECT_LIST_H__
#define __DECOR_OBJECT_LIST_H__

#include "./decor_object.h"

#define DECOR_TYPE_CYLINDER 0
#define DECOR_TYPE_RADIO    1
#define DECOR_TYPE_CUBE     2

struct DecorObjectDefinition* decorObjectDefinitionForId(int id);

#endif