#ifndef __MATERIAL_ENUMS_H__
#define __MATERIAL_ENUMS_H__

enum class GeometryMode {
    None = 0,
    G_ZBUFFER = (1 << 0),
    G_SHADE = (1 << 1),
    G_TEXTURE_ENABLE = (1 << 2),
    G_SHADING_SMOOTH = (1 << 3),
    G_CULL_FRONT = (1 << 4),
    G_CULL_BACK = (1 << 5),
    G_FOG = (1 << 6),
    G_LIGHTING = (1 << 7),
    G_TEXTURE_GEN = (1 << 8),
    G_TEXTURE_GEN_LINEAR = (1 << 9),
    G_LOD = (1 << 10),
    G_CLIPPING = (1 << 11),
};

extern const char* gGeometryModeNames[];

#define GEOMETRY_MODE_COUNT 12

enum class CycleType {
    Unknown,
    _1Cycle,
    _2Cycle,
    Copy,
    Fill,
    Count,
};

extern const char* gCycleTypeNames[];

enum class ColorCombineSource {
    Combined,
    Texel0,
    Texel1,
    PrimitiveColor,
    ShadeColor,
    EnvironmentColor,
    KeyCenter,
    KeyScale,
    CombinedAlpha,
    Texture0Alpha,
    Texture1Alpha,
    PrimitiveAlpha,
    ShadedAlpha,
    EnvironmentAlpha,
    LODFraction,
    PrimitiveLODFraction,
    Noise,
    ConvertK4,
    ConvertK5,
    _1,
    _0,
    Count,
};

extern const char* gColorCombineSourceNames[];

bool canUseColorCombineSource(int offset, ColorCombineSource source);

enum class AlphaCombineSource {
    CombinedAlpha,
    Texture0Alpha,
    Texture1Alpha,
    PrimitiveAlpha,
    ShadedAlpha,
    EnvironmentAlpha,
    LODFraction,
    PrimitiveLODFraction,
    _1,
    _0,
    Count,
};

extern const char* gAlphaCombineSourceNames[];

bool canUseAlphaCombineSource(int offset, AlphaCombineSource source);

enum class PipelineMode {
    Unknown,
    _1Primitive,
    _NPrimitive,
    Count,
};

extern const char* gPipelineModeNames[];

enum class PerspectiveMode {
    Unknown,
    None,
    Perspective,
    Count,
};

extern const char* gPerspectiveModeNames[];

enum class TextureDetail {
    Unknown,
    Clamp,
    Sharpen,
    Detail,
    Count,
};

extern const char* gTextureDetailNames[];

enum class TextureLOD {
    Unknown,
    Tile,
    LOD,
    Count,
};

extern const char* gTextureLODNames[];

enum class TextureLUT {
    Unknown,
    None,
    RGBA16,
    IA16,
    Count,
};

extern const char* gTextureLUTNames[];

enum class TextureFilter {
    Unknown,
    Point,
    Average,
    Bilerp,
    Count,
};

extern const char* gTextureFilterNames[];

enum class TextureConvert {
    Unknown,
    Conv,
    FiltConv,
    Filt,
    Count,
};

extern const char* gTextureConvertNames[];

enum class CombineKey {
    Unknown,
    None,
    Key,
    Count,
};

extern const char* gCombineKeyNames[];

enum class ColorDither {
    Unknown,
    MagicSQ,
    Bayer,
    Noise,
    Disable,
    Count,
};

extern const char* gCotherDitherNames[];

enum class AlphaDither {
    Unknown,
    Pattern,
    NotPattern,
    Noise,
    Disable,
    Count,
};

extern const char* gAlphaDitherNames[];

enum class AlphaCompare {
    Unknown,
    None,
    Threshold,
    Dither,
    Count,
};

extern const char* gAlphaCompareNames[];

enum class DepthSource {
    Unknown,
    Pixel,
    Primitive,
    Count,
};

extern const char* gDepthSourceNames[];

#endif