Vtx ground_Plane_normal[] = {
    {{{499, 0, 499}, 0, {0, 4096}, {0, 127, 0, 255}}},
    {{{499, 0, -499}, 0, {4096, 4096}, {0, 127, 0, 255}}},
    {{{-499, 0, -499}, 0, {4096, 0}, {0, 127, 0, 255}}},
    {{{-499, 0, 499}, 0, {0, 0}, {0, 127, 0, 255}}},
};


Gfx ground_model_gfx[] = {
    // Material DefaultMaterial
    // End Material DefaultMaterial
    gsSPVertex(&ground_Plane_normal[0], 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};