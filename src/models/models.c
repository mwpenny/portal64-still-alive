#include <ultra64.h>

#include "sk64/skelatool_animator.h"

#include "portal_mask.inc.h"
#include "portal_outline.inc.h"

#include "../../build/assets/models/cube/cube.h"
#include "../../build/assets/models/portal_gun/v_portalgun.h"
#include "../../build/assets/materials/static.h"

Gfx* cube_gfx = &cube_cube_model_gfx[0];
short cube_material_index = CUBE_INDEX;
Gfx* v_portal_gun_gfx = &portal_gun_v_portalgun_model_gfx[0];