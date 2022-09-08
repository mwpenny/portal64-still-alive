#ifndef __MATERIAL_STATE_H__
#define __MATERIAL_STATE_H__

#include <inttypes.h>
#include "TextureDefinition.h"
#include "MaterialEnums.h"
#include "../CFileDefinition.h"

struct FlagList {
    FlagList();
    uint64_t flags;
    uint64_t knownFlags;

    void SetFlag(int mask, bool value);
    void DeleteFlag(int mask);

    struct FlagList GetDeltaFrom(struct FlagList& other);

    void ApplyFrom(const FlagList& other);
};

struct TextureCoordinateState {
public:
    TextureCoordinateState();
    bool wrap;
    bool mirror;
    int mask;
    int shift;
    int offset;
    int limit;
};

struct TileState {
public:
    TileState();

    bool IsTileStateEqual(const TileState& other) const;
    bool IsTileSizeEqual(const TileState& other) const;
    
    bool isOn;
    std::shared_ptr<TextureDefinition> texture;
    G_IM_FMT format;
    G_IM_SIZ size;
    // 1 line is a 64 bit offset in TMEM
    int line;
    int tmem;
    int pallete;
    struct TextureCoordinateState sCoord;
    struct TextureCoordinateState tCoord;
};

struct TextureState {
public:
    TextureState();
    uint16_t sc;
    uint16_t tc;

    int level;
    int tile;

    bool isOn;
};

struct ColorCombineMode {
    ColorCombineMode();
    ColorCombineMode(ColorCombineSource c0, ColorCombineSource c1, ColorCombineSource c2, ColorCombineSource c3, AlphaCombineSource a0, AlphaCombineSource a1, AlphaCombineSource a2, AlphaCombineSource a3);

    bool operator==(const ColorCombineMode& other) const;

    ColorCombineSource color[4];
    AlphaCombineSource alpha[4];
};

struct RenderModeState {
public:
    RenderModeState();
    RenderModeState(int data);

    bool operator==(const RenderModeState& other) const;

    int GetZMode() const;
    
    int data;
};

bool findRenderModeByName(const std::string& name, RenderModeState& output);

#define MAX_TILE_COUNT  8

struct MaterialState {
public:
    MaterialState();
    // state to keep track of
    // tiles
    // RDP tile cache
    struct TileState tiles[MAX_TILE_COUNT];
    struct TextureState textureState;

    // geometry modes
    FlagList geometryModes;

    // G_SETOTHERMODE_H
    PipelineMode pipelineMode;
    CycleType cycleType;
    PerspectiveMode perspectiveMode;
    TextureDetail textureDetail;
    TextureLOD textureLOD;
    TextureLUT textureLUT;
    TextureFilter textureFilter;
    TextureConvert textureConvert;
    CombineKey combineKey;
    ColorDither colorDither;
    AlphaDither alphaDither;
    // G_SETOTHERMODE_L
    AlphaCompare alphaCompare;
    DepthSource depthSource;

    // combine mode
    bool hasCombineMode;
    ColorCombineMode cycle1Combine;
    ColorCombineMode cycle2Combine;

    bool hasRenderMode;
    RenderModeState cycle1RenderMode;
    RenderModeState cycle2RenderMode;

    // RDP colors
    bool usePrimitiveColor;
    PixelRGBAu8 primitiveColor;
    uint8_t primitiveM;
    uint8_t primitiveL;

    bool useEnvColor;
    PixelRGBAu8 envColor;

    bool useFillColor;
    PixelRGBAu8 fillColor;
    
    bool useFogColor;
    PixelRGBAu8 fogColor;

    bool useBlendColor;
    PixelRGBAu8 blendColor;

    bool IsTextureLoaded(std::shared_ptr<TextureDefinition> texture, int tmem) const;
};

void generateMaterial(CFileDefinition& fileDef, const MaterialState& from, const MaterialState& to, StructureDataChunk& output, bool targetCIBuffer);

void applyMaterial(const MaterialState& from, MaterialState& to);

#endif