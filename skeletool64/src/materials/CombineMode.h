#ifndef __COMBINE_MODE_H__
#define __COMBINE_MODE_H__

#include "MaterialState.h"
#include <string>

/*
 * G_SETCOMBINE: color combine modes
 */
/* Color combiner constants: */
#define G_CCMUX_COMBINED	ColorCombineSource::Combined
#define G_CCMUX_TEXEL0		ColorCombineSource::Texel0
#define G_CCMUX_TEXEL1		ColorCombineSource::Texel1
#define G_CCMUX_PRIMITIVE	ColorCombineSource::PrimitiveColor
#define G_CCMUX_SHADE		ColorCombineSource::ShadeColor
#define G_CCMUX_ENVIRONMENT	ColorCombineSource::EnvironmentColor
#define G_CCMUX_CENTER		ColorCombineSource::KeyCenter
#define G_CCMUX_SCALE		ColorCombineSource::KeyScale
#define G_CCMUX_COMBINED_ALPHA	ColorCombineSource::CombinedAlpha
#define G_CCMUX_TEXEL0_ALPHA	ColorCombineSource::Texture0Alpha
#define G_CCMUX_TEXEL1_ALPHA	ColorCombineSource::Texture1Alpha
#define G_CCMUX_PRIMITIVE_ALPHA	ColorCombineSource::PrimitiveAlpha
#define G_CCMUX_SHADE_ALPHA	ColorCombineSource::ShadedAlpha
#define G_CCMUX_ENV_ALPHA	ColorCombineSource::EnvironmentAlpha
#define G_CCMUX_LOD_FRACTION	ColorCombineSource::LODFraction
#define G_CCMUX_PRIM_LOD_FRAC	ColorCombineSource::PrimitiveLODFraction
#define G_CCMUX_NOISE		ColorCombineSource::Noise
#define G_CCMUX_K4		ColorCombineSource::ConvertK4
#define G_CCMUX_K5		ColorCombineSource::ConvertK5
#define G_CCMUX_1	    ColorCombineSource::_1
#define G_CCMUX_0		ColorCombineSource::_0

/* Alpha combiner constants: */
#define G_ACMUX_COMBINED	AlphaCombineSource::CombinedAlpha
#define G_ACMUX_TEXEL0		AlphaCombineSource::Texture0Alpha
#define G_ACMUX_TEXEL1		AlphaCombineSource::Texture1Alpha
#define G_ACMUX_PRIMITIVE	AlphaCombineSource::PrimitiveAlpha
#define G_ACMUX_SHADE		AlphaCombineSource::ShadedAlpha
#define G_ACMUX_ENVIRONMENT	AlphaCombineSource::EnvironmentAlpha
#define G_ACMUX_LOD_FRACTION	AlphaCombineSource::LODFraction
#define G_ACMUX_PRIM_LOD_FRAC	AlphaCombineSource::PrimitiveLODFraction
#define G_ACMUX_1		AlphaCombineSource::_1
#define G_ACMUX_0		AlphaCombineSource::_0

/* typical CC cycle 1 modes */
#define	G_CC_PRIMITIVE		0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE
#define	G_CC_SHADE		0, 0, 0, SHADE, 0, 0, 0, SHADE
#define	G_CC_MODULATEI		TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE
#define	G_CC_MODULATEIA		TEXEL0, 0, SHADE, 0, TEXEL0, 0, SHADE, 0
#define	G_CC_MODULATEIDECALA	TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0
#define	G_CC_MODULATERGB	G_CC_MODULATEI
#define	G_CC_MODULATERGBA	G_CC_MODULATEIA
#define	G_CC_MODULATERGBDECALA	G_CC_MODULATEIDECALA
#define	G_CC_MODULATEI_PRIM	TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define	G_CC_MODULATEIA_PRIM	TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0
#define	G_CC_MODULATEIDECALA_PRIM	TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0
#define	G_CC_MODULATERGB_PRIM	G_CC_MODULATEI_PRIM
#define	G_CC_MODULATERGBA_PRIM	G_CC_MODULATEIA_PRIM
#define	G_CC_MODULATERGBDECALA_PRIM	G_CC_MODULATEIDECALA_PRIM
#define	G_CC_DECALRGB		0, 0, 0, TEXEL0, 0, 0, 0, SHADE
#define	G_CC_DECALRGBA		0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0
#define	G_CC_BLENDI		ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDIA		ENVIRONMENT, SHADE, TEXEL0, SHADE, TEXEL0, 0, SHADE, 0
#define	G_CC_BLENDIDECALA	ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define	G_CC_BLENDRGBA		TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDRGBDECALA	TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, TEXEL0
#define G_CC_ADDRGB		1, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_ADDRGBDECALA	1, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_REFLECTRGB		ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_REFLECTRGBDECALA	ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_HILITERGB		PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_HILITERGBA		PRIMITIVE, SHADE, TEXEL0, SHADE, PRIMITIVE, SHADE, TEXEL0, SHADE
#define G_CC_HILITERGBDECALA	PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_SHADEDECALA	0, 0, 0, SHADE, 0, 0, 0, TEXEL0
#define	G_CC_BLENDPE		PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0
#define	G_CC_BLENDPEDECALA	PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, TEXEL0

/* oddball modes */
#define	_G_CC_BLENDPE		ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, TEXEL0, 0, SHADE, 0
#define	_G_CC_BLENDPEDECALA	ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, 0, 0, 0, TEXEL0
#define	_G_CC_TWOCOLORTEX	PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
/* used for 1-cycle sparse mip-maps, primitive color has color of lowest LOD */
#define	_G_CC_SPARSEST		PRIMITIVE, TEXEL0, LOD_FRACTION, TEXEL0, PRIMITIVE, TEXEL0, LOD_FRACTION, TEXEL0
#define G_CC_TEMPLERP   TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0

/* typical CC cycle 1 modes, usually followed by other cycle 2 modes */
#define	G_CC_TRILERP		TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0
#define	G_CC_INTERFERENCE	TEXEL0, 0, TEXEL1, 0, TEXEL0, 0, TEXEL1, 0

/*
 *  One-cycle color convert operation
 */
#define	G_CC_1CYUV2RGB		TEXEL0, K4, K5, TEXEL0, 0, 0, 0, SHADE

/*
 *  NOTE: YUV2RGB expects TF step1 color conversion to occur in 2nd clock.
 * Therefore, CC looks for step1 results in TEXEL1
 */
#define	G_CC_YUV2RGB		TEXEL1, K4, K5, TEXEL1, 0, 0, 0, 0

/* typical CC cycle 2 modes */
#define	G_CC_PASS2		0, 0, 0, COMBINED, 0, 0, 0, COMBINED
#define	G_CC_MODULATEI2		COMBINED, 0, SHADE, 0, 0, 0, 0, SHADE
#define	G_CC_MODULATEIA2	COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0
#define	G_CC_MODULATERGB2	G_CC_MODULATEI2
#define	G_CC_MODULATERGBA2	G_CC_MODULATEIA2
#define	G_CC_MODULATEI_PRIM2	COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define	G_CC_MODULATEIA_PRIM2	COMBINED, 0, PRIMITIVE, 0, COMBINED, 0, PRIMITIVE, 0
#define	G_CC_MODULATERGB_PRIM2	G_CC_MODULATEI_PRIM2
#define	G_CC_MODULATERGBA_PRIM2	G_CC_MODULATEIA_PRIM2
#define	G_CC_DECALRGB2		0, 0, 0, COMBINED, 0, 0, 0, SHADE
/*
 * ?
#define	G_CC_DECALRGBA2		COMBINED, SHADE, COMBINED_ALPHA, SHADE, 0, 0, 0, SHADE
*/
#define	G_CC_BLENDI2		ENVIRONMENT, SHADE, COMBINED, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDIA2		ENVIRONMENT, SHADE, COMBINED, SHADE, COMBINED, 0, SHADE, 0
#define	G_CC_CHROMA_KEY2	TEXEL0, CENTER, SCALE, 0, 0, 0, 0, 0
#define G_CC_HILITERGB2		ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, SHADE
#define G_CC_HILITERGBA2	ENVIRONMENT, COMBINED, TEXEL0, COMBINED, ENVIRONMENT, COMBINED, TEXEL0, COMBINED
#define G_CC_HILITERGBDECALA2	ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, TEXEL0
#define G_CC_HILITERGBPASSA2	ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, COMBINED

#define DEFINE_COMBINE_MODE_LERP(c0, c1, c2, c3, a0, a1, a2, a3) \
    ColorCombineMode(G_CCMUX_##c0, G_CCMUX_##c1, G_CCMUX_##c2, G_CCMUX_##c3, G_ACMUX_##a0, G_ACMUX_##a1, G_ACMUX_##a2, G_ACMUX_##a3)

#define DEFINE_COMBINE_MODE(terms) DEFINE_COMBINE_MODE_LERP(terms)

bool combineModeWithName(const std::string& name, ColorCombineMode& result);

#endif