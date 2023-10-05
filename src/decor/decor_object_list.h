#ifndef __DECOR_OBJECT_LIST_H__
#define __DECOR_OBJECT_LIST_H__

#include "./decor_object.h"

#define DECOR_TYPE_CYLINDER             0
#define DECOR_TYPE_RADIO                1
#define DECOR_TYPE_CUBE                 2
#define DECOR_TYPE_CUBE_UNIMPORTANT     3
#define DECOR_TYPE_AUTOPORTAL_FRAME     4
#define DECOR_TYPE_LIGHT_RAIL_ENDCAP    5

struct DecorObjectDefinition* decorObjectDefinitionForId(int id);
int decorIdForObjectDefinition(struct DecorObjectDefinition* def);

#endif