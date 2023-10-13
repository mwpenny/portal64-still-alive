#ifndef __DECOR_OBJECT_LIST_H__
#define __DECOR_OBJECT_LIST_H__

#include "./decor_object.h"

#define DECOR_TYPE_CYLINDER             0
#define DECOR_TYPE_RADIO                1
#define DECOR_TYPE_CUBE                 2
#define DECOR_TYPE_CUBE_UNIMPORTANT     3
#define DECOR_TYPE_AUTOPORTAL_FRAME     4
#define DECOR_TYPE_LIGHT_RAIL_ENDCAP    5
#define DECOR_TYPE_LAB_MONITOR          6
#define DECOR_TYPE_LAB_CHAIR            7
#define DECOR_TYPE_LAB_DESK01           8
#define DECOR_TYPE_LAB_DESK02           8
#define DECOR_TYPE_LAB_DESK03           10
#define DECOR_TYPE_LAB_DESK04           11

struct DecorObjectDefinition* decorObjectDefinitionForId(int id);
int decorIdForObjectDefinition(struct DecorObjectDefinition* def);

#endif