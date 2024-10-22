#ifndef __RENDER_MODE_H__
#define __RENDER_MODE_H__

#include <vector>
#include <string>

#define	AA_EN		0x8
#define	Z_CMP		0x10
#define	Z_UPD		0x20
#define	IM_RD		0x40
#define	CLR_ON_CVG	0x80
#define	CVG_DST_CLAMP	0
#define	CVG_DST_WRAP	0x100
#define	CVG_DST_FULL	0x200
#define	CVG_DST_SAVE	0x300
#define	CVG_DST_MASK	0x300
#define	ZMODE_OPA	0
#define	ZMODE_INTER	0x400
#define	ZMODE_XLU	0x800
#define	ZMODE_DEC	0xc00
#define	ZMODE_MASK	0xc00
#define	CVG_X_ALPHA	0x1000
#define	ALPHA_CVG_SEL	0x2000
#define	FORCE_BL	0x4000
#define	TEX_EDGE	0x0000 /* used to be 0x8000 */

#define	G_BL_CLR_IN	0
#define	G_BL_CLR_MEM	1
#define	G_BL_CLR_BL	2
#define	G_BL_CLR_FOG	3
#define	G_BL_1MA	0
#define	G_BL_A_MEM	1
#define	G_BL_A_IN	0
#define	G_BL_A_FOG	1
#define	G_BL_A_SHADE	2
#define	G_BL_1		2
#define	G_BL_0		3

#define	G_MDSFT_ALPHACOMPARE		0

#define	G_AC_NONE		(0 << G_MDSFT_ALPHACOMPARE)
#define	G_AC_THRESHOLD		(1 << G_MDSFT_ALPHACOMPARE)
#define	G_AC_DITHER		(3 << G_MDSFT_ALPHACOMPARE)

#define	GBL_c1(m1a, m1b, m2a, m2b)	\
	(m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18

#define G_RM_AA_ZB_OPA_SURF					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_RA_ZB_OPA_SURF					\
	AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |			\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_ZB_XLU_SURF					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_XLU |					\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_OPA_DECAL					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | ALPHA_CVG_SEL |	\
	ZMODE_DEC |						\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_RA_ZB_OPA_DECAL					\
	AA_EN | Z_CMP | CVG_DST_WRAP | ALPHA_CVG_SEL |		\
	ZMODE_DEC |						\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_ZB_XLU_DECAL					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_DEC |					\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_OPA_INTER					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ALPHA_CVG_SEL |	ZMODE_INTER |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_RA_ZB_OPA_INTER					\
	AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |			\
	ALPHA_CVG_SEL |	ZMODE_INTER |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_ZB_XLU_INTER					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_INTER |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_XLU_LINE					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |	\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_XLU |			\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_DEC_LINE					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_SAVE | CVG_X_ALPHA |	\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_DEC |			\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_TEX_EDGE					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_ZB_TEX_INTER					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_INTER | TEX_EDGE |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_ZB_SUB_SURF					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_ZB_PCL_SURF					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | G_AC_DITHER | 				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_OPA_TERR					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_TEX_TERR					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_ZB_SUB_TERR					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define G_RM_AA_OPA_SURF					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_RA_OPA_SURF					\
	AA_EN | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_XLU_SURF					\
	AA_EN | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL |	\
	ZMODE_OPA |						\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_XLU_LINE					\
	AA_EN | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |		\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |			\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_DEC_LINE					\
	AA_EN | IM_RD | CVG_DST_FULL | CVG_X_ALPHA |		\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |			\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_TEX_EDGE					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_SUB_SURF					\
	AA_EN | IM_RD | CVG_DST_FULL |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define G_RM_AA_PCL_SURF					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | G_AC_DITHER | 				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_OPA_TERR					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_TEX_TERR					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_AA_SUB_TERR					\
	AA_EN | IM_RD | CVG_DST_FULL |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define G_RM_ZB_OPA_SURF					\
	Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL |		\
	ZMODE_OPA |						\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)
	
#define G_RM_ZB_XLU_SURF					\
	Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_XLU |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
	
#define G_RM_ZB_OPA_DECAL					\
	Z_CMP | CVG_DST_FULL | ALPHA_CVG_SEL | ZMODE_DEC |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)
	
#define G_RM_ZB_XLU_DECAL					\
	Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_DEC |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
	
#define G_RM_ZB_CLD_SURF					\
	Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_XLU |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
	
#define G_RM_ZB_OVL_SURF					\
	Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_DEC |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
	
#define G_RM_ZB_PCL_SURF					\
	Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA |		\
	G_AC_DITHER | 						\
	GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)


#define G_RM_OPA_SURF					\
	CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA |			\
	GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define G_RM_XLU_SURF					\
	IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA |		\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_TEX_EDGE					\
	CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL |\
	ZMODE_OPA | TEX_EDGE | AA_EN |					\
	GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define G_RM_CLD_SURF					\
	IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |		\
	GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define G_RM_PCL_SURF					\
	CVG_DST_FULL | FORCE_BL | ZMODE_OPA | 			\
	G_AC_DITHER | 						\
	GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define G_RM_ADD					\
	IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |	\
	GBL_c1(G_BL_CLR_IN, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1)

#define G_RM_NOOP	\
	GBL_c1(0, 0, 0, 0)

#define G_RM_VISCVG \
	IM_RD | FORCE_BL |     \
	GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_BL, G_BL_A_MEM)

/* for rendering to an 8-bit framebuffer */
#define G_RM_OPA_CI                    \
	CVG_DST_CLAMP | ZMODE_OPA |          \
	GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	G_RM_FOG_SHADE_A	GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_FOG_PRIM_A		GBL_c1(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_PASS		GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

void renderModeExtractFlags(int flags, std::vector<std::string>& output);
bool renderModeGetFlagValue(const std::string& name, int& output);
bool renderModeGetBlendModeValue(const std::string& name, int index, int& output);
const std::string& renderModeGetBlendModeName(int blendMode, int index);

#endif