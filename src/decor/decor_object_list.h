#ifndef __DECOR_OBJECT_LIST_H__
#define __DECOR_OBJECT_LIST_H__

#include "decor_object.h"

#define DECOR_TYPE_CYLINDER             0
#define DECOR_TYPE_RADIO                1
#define DECOR_TYPE_CUBE                 2
#define DECOR_TYPE_CUBE_UNIMPORTANT     3
#define DECOR_TYPE_COMPANION_CUBE       4
#define DECOR_TYPE_AUTOPORTAL_FRAME     5
#define DECOR_TYPE_LIGHT_RAIL_ENDCAP    6
#define DECOR_TYPE_LAB_MONITOR          7
#define DECOR_TYPE_LAB_CHAIR            8
#define DECOR_TYPE_LAB_DESK01           9
#define DECOR_TYPE_LAB_DESK02           10
#define DECOR_TYPE_LAB_DESK03           11
#define DECOR_TYPE_LAB_DESK04           12
#define DECOR_TYPE_SCRAWLINGS002A       13
#define DECOR_TYPE_FOOD_CAN             14
#define DECOR_TYPE_WATER_BOTTLE         15
#define DECOR_TYPE_SAUCEPAN             16
#define DECOR_TYPE_METALBUCKET01A       17
#define DECOR_TYPE_MILK_CARTON          18
#define DECOR_TYPE_MILK_CARTON_OPEN     19
#define DECOR_TYPE_PC_CASE_OPEN         20

struct DecorObjectDefinition* decorObjectDefinitionForId(int id);
int decorIdForObjectDefinition(struct DecorObjectDefinition* def);

int decorIdForCollisionObject(struct CollisionObject* collisionObject); // evil hack

#endif