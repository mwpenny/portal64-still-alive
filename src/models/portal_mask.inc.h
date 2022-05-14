Lights1 portal_mask_polygon_portal_lights = gdSPDefLights1(
	0x7F, 0x7F, 0x7F,
	0xFE, 0xFE, 0xFE, 0x28, 0x28, 0x28);

Vtx portal_mask_Circle_mesh_vtx_0[8] = {
	{{{-106, 0, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{-75, 150, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{0, 213, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{106, 0, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{75, 150, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{0, -213, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{75, -150, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
	{{{-75, -150, 0},0, {-16, 1008},{0x0, 0x0, 0x81, 0xFF}}},
};

Gfx portal_mask_Circle_mesh_tri_0[] = {
	gsSPVertex(portal_mask_Circle_mesh_vtx_0 + 0, 8, 0),
	gsSP1Triangle(0, 1, 2, 0),
	gsSP1Triangle(0, 2, 3, 0),
	gsSP1Triangle(2, 4, 3, 0),
	gsSP1Triangle(3, 5, 0, 0),
	gsSP1Triangle(3, 6, 5, 0),
	gsSP1Triangle(5, 7, 0, 0),
	gsSPEndDisplayList(),
};


#define	RM_UPDATE_Z(clk)		\
    Z_CMP | Z_UPD | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_XLU |                          \
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

Gfx mat_portal_mask_portal_mask[] = {
	gsDPPipeSync(),
    gsDPSetRenderMode(RM_UPDATE_Z(1), RM_UPDATE_Z(2)),
	gsDPSetCombineLERP(0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE),
	gsDPSetPrimColor(0, 0, 255, 128, 0, 0),
	gsSPEndDisplayList(),
};

Gfx portal_mask_Circle_mesh[] = {
	gsSPDisplayList(mat_portal_mask_portal_mask),
	gsSPDisplayList(portal_mask_Circle_mesh_tri_0),
	gsDPPipeSync(),
    // gsSPGeometryMode(G_CULL_BACK, G_CULL_FRONT),
	// gsDPSetPrimColor(0, 0, 0, 128, 255, 0),
	// gsSP1Triangle(0, 1, 2, 0),
	// gsSP1Triangle(0, 2, 3, 0),
	// gsSP1Triangle(2, 4, 3, 0),
	// gsSP1Triangle(3, 5, 0, 0),
	// gsSP1Triangle(3, 6, 5, 0),
	// gsSP1Triangle(5, 7, 0, 0),
	// gsDPPipeSync(),
    // gsSPGeometryMode(G_CULL_FRONT, G_CULL_BACK),
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPClearGeometryMode(G_TEXTURE_GEN),
	gsDPSetCombineLERP(0, 0, 0, SHADE, 0, 0, 0, ENVIRONMENT, 0, 0, 0, SHADE, 0, 0, 0, ENVIRONMENT),
	gsSPTexture(65535, 65535, 0, 0, 0),
	gsSPEndDisplayList(),
};
