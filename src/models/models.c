#include <ultra64.h>

#include "sk64/skelatool_animator.h"

#include "../../build/assets/models/portal_gun/v_portalgun.h"
#include "../../build/assets/materials/static.h"
#include "../../build/assets/models/props/button.h"
#include "../../build/assets/models/props/door_01.h"
#include "../../build/assets/models/props/cylinder_test.h"

Gfx* v_portal_gun_gfx = &portal_gun_v_portalgun_model_gfx[0];

Gfx* button_gfx = &props_button_model_gfx[0];
short button_material_index = BUTTON_INDEX;

Gfx* door_01_gfx = &props_door_01_model_gfx[0];
short door_01_material_index = DOOR_01_INDEX;

Gfx* cylinder_gfx = &props_cylinder_test_model_gfx[0];
short cylinder_material_index = PLASTIC_PLASTICWALL001A_INDEX;

short fizzler_material_index = PORTAL_CLEANSER_INDEX;