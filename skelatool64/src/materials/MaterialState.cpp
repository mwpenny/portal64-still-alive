#include "MaterialState.h"

#include "MaterialEnums.h"

#include "RenderMode.h"
#include <iostream>


FlagList::FlagList() : flags(0), knownFlags(0) {}

void FlagList::SetFlag(int mask, bool value) {
    knownFlags |= mask;

    if (value) {
        flags |= mask;
    } else {
        flags &= ~mask;
    }
}

void FlagList::DeleteFlag(int mask) {
    flags &= ~mask;
    knownFlags &= ~mask;
}

struct FlagList FlagList::GetDeltaFrom(struct FlagList& other) {
    struct FlagList result;

    result.knownFlags = 
        // flags known by both with different values
        (knownFlags & other.knownFlags & (flags ^ other.flags)) | 
        // flags known by this but not that
        (knownFlags & ~other.knownFlags);
    // mask by knownFlags so unknown data is set
    // to 0 to avoid confusion
    result.flags = flags & result.knownFlags;

    return result;
}

TextureCoordinateState::TextureCoordinateState():
    wrap(true),
    mirror(false),
    mask(0),
    shift(0),
    offset(0),
    limit(0) {

}

TileState::TileState():
    isOn(false),
    line(0),
    pallete(0) {

}

bool TileState::IsTileStateEqual(const TileState& other) const {
    return format == other.format &&
        size == other.size &&
        line == other.line &&
        tmem == other.tmem &&
        pallete == other.pallete &&
        sCoord.wrap == other.sCoord.wrap &&
        sCoord.mirror == other.sCoord.mirror &&
        sCoord.mask == other.sCoord.mask &&
        sCoord.shift == other.sCoord.shift &&
        tCoord.wrap == other.tCoord.wrap &&
        tCoord.mirror == other.tCoord.mirror &&
        tCoord.mask == other.tCoord.mask &&
        tCoord.shift == other.tCoord.shift;
}

bool TileState::IsTileSizeEqual(const TileState& other) const {
    return sCoord.offset == other.sCoord.offset &&
        sCoord.limit == other.sCoord.limit &&
        tCoord.offset == other.tCoord.offset &&
        tCoord.limit == other.tCoord.limit;
}

TextureState::TextureState():
    sc(0xFFFF),
    tc(0xFFFF),
    level(0),
    tile(0),
    isOn(false) {}

ColorCombineMode::ColorCombineMode() : 
    color{ColorCombineSource::_0, ColorCombineSource::_0, ColorCombineSource::_0, ColorCombineSource::_0},
    alpha{AlphaCombineSource::_0, AlphaCombineSource::_0, AlphaCombineSource::_0, AlphaCombineSource::_0} {}


ColorCombineMode::ColorCombineMode(ColorCombineSource c0, ColorCombineSource c1, ColorCombineSource c2, ColorCombineSource c3, AlphaCombineSource a0, AlphaCombineSource a1, AlphaCombineSource a2, AlphaCombineSource a3) : 
    color{c0, c1, c2, c3},
    alpha{a0, a1, a2, a3} {}

bool ColorCombineMode::operator==(const ColorCombineMode& other) const {
    return color[0] == other.color[0] &&
        color[1] == other.color[1] &&
        color[2] == other.color[2] &&
        color[3] == other.color[3] &&
        alpha[0] == other.alpha[0] &&
        alpha[1] == other.alpha[1] &&
        alpha[2] == other.alpha[2] &&
        alpha[3] == other.alpha[3];
}

RenderModeState::RenderModeState() : data(G_RM_OPA_SURF) {
    
}

RenderModeState::RenderModeState(int data) : data(data) {};

bool RenderModeState::operator==(const RenderModeState& other) const {
    return data == other.data;
}

#define DEFINE_RENDER_MODE_ENTRY(name)  std::make_pair(std::string(#name), RenderModeState(name))

std::pair<std::string, RenderModeState> gRenderModes[] = {
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_OPA_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_RA_ZB_OPA_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_XLU_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_OPA_DECAL),
    DEFINE_RENDER_MODE_ENTRY(G_RM_RA_ZB_OPA_DECAL),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_XLU_DECAL),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_OPA_INTER),
    DEFINE_RENDER_MODE_ENTRY(G_RM_RA_ZB_OPA_INTER),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_XLU_INTER),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_XLU_LINE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_DEC_LINE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_TEX_EDGE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_TEX_INTER),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_SUB_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_PCL_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_OPA_TERR),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_TEX_TERR),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_ZB_SUB_TERR),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_OPA_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_RA_OPA_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_XLU_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_XLU_LINE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_DEC_LINE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_TEX_EDGE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_SUB_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_PCL_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_OPA_TERR),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_TEX_TERR),
    DEFINE_RENDER_MODE_ENTRY(G_RM_AA_SUB_TERR),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_OPA_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_XLU_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_OPA_DECAL),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_XLU_DECAL),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_CLD_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_OVL_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ZB_PCL_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_OPA_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_XLU_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_TEX_EDGE),
    DEFINE_RENDER_MODE_ENTRY(G_RM_CLD_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_PCL_SURF),
    DEFINE_RENDER_MODE_ENTRY(G_RM_ADD),
    DEFINE_RENDER_MODE_ENTRY(G_RM_NOOP),
    DEFINE_RENDER_MODE_ENTRY(G_RM_VISCVG),
    DEFINE_RENDER_MODE_ENTRY(G_RM_OPA_CI),
    DEFINE_RENDER_MODE_ENTRY(G_RM_FOG_SHADE_A),
    DEFINE_RENDER_MODE_ENTRY(G_RM_FOG_PRIM_A),
    DEFINE_RENDER_MODE_ENTRY(G_RM_PASS),
};


bool findRenderModeByName(const std::string& name, RenderModeState& output) {
    for (auto pair : gRenderModes) {
        if (pair.first == name) {
            output = pair.second;
            return true;
        }
    }

    return false;
}

MaterialState::MaterialState() :
    pipelineMode(PipelineMode::Unknown),
    cycleType(CycleType::Unknown),
    perspectiveMode(PerspectiveMode::Unknown),
    textureDetail(TextureDetail::Unknown),
    textureLOD(TextureLOD::Unknown),
    textureLUT(TextureLUT::Unknown),
    textureFilter(TextureFilter::Unknown),
    textureConvert(TextureConvert::Unknown),
    combineKey(CombineKey::Unknown),
    colorDither(ColorDither::Unknown),
    alphaDither(AlphaDither::Unknown),
    alphaCompare(AlphaCompare::Unknown),
    depthSource(DepthSource::Unknown),
    hasCombineMode(false),
    hasRenderMode(false),
    usePrimitiveColor(false),
    primitiveM(255),
    primitiveL(255),
    useEnvColor(false),
    useFillColor(false),
    useFogColor(false),
    useBlendColor(false)
     {}

void appendToFlags(std::ostringstream& flags, const std::string& value) {
    if (flags.tellp() != 0) {
        flags << " | ";
    }
    flags << value;
}

std::unique_ptr<DataChunk> generateGeometryModes(const MaterialState& from, const MaterialState& to) {
    std::ostringstream clearFlags;
    std::ostringstream setFlags;

    for (int i = 0; i < GEOMETRY_MODE_COUNT; ++i) {
        int mask = 1 << i;

        bool isKnownToTarget = (to.geometryModes.knownFlags & mask) != 0;
        bool isKnownToSource = (from.geometryModes.knownFlags & mask) != 0;
        bool targetMatchesSource = (mask & (to.geometryModes.flags ^ from.geometryModes.flags)) == 0;

        if (isKnownToTarget && (!isKnownToSource || !targetMatchesSource)) {
            if (to.geometryModes.flags & mask) {
                appendToFlags(setFlags, gGeometryModeNames[i]);
            } else {
                appendToFlags(clearFlags, gGeometryModeNames[i]);
            }
        }
    }

    if (clearFlags.tellp() == 0 && setFlags.tellp() == 0) {
        return NULL;
    }

    if (clearFlags.tellp() == 0) {
        clearFlags << "0";
    }

    if (setFlags.tellp() == 0) {
        setFlags << "0";
    }

    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPGeometryMode"));

    result->AddPrimitive(clearFlags.str());
    result->AddPrimitive(setFlags.str());

    return result;
}

void generateEnumMacro(int from, int to, const char* macroName, const char** options, StructureDataChunk& output) {
    if (from == to || to == 0) {
        return;
    }

    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk(macroName));

    result->AddPrimitive(options[to]);

    output.Add(std::move(result));
}

std::unique_ptr<DataChunk> generateCombineMode(const MaterialState& from, const MaterialState& to) {
    if (!to.hasCombineMode || 
        (from.hasCombineMode && from.cycle1Combine == to.cycle1Combine && from.cycle2Combine  == to.cycle2Combine)) {
        return NULL;
    }

    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsDPSetCombineLERP"));

    for (int i = 0; i < 4; ++i) {
        result->AddPrimitive(gColorCombineSourceNames[(int)to.cycle1Combine.color[i]]);
    }

    for (int i = 0; i < 4; ++i) {
        result->AddPrimitive(gAlphaCombineSourceNames[(int)to.cycle1Combine.alpha[i]]);
    }

    for (int i = 0; i < 4; ++i) {
        result->AddPrimitive(gColorCombineSourceNames[(int)to.cycle2Combine.color[i]]);
    }

    for (int i = 0; i < 4; ++i) {
        result->AddPrimitive(gAlphaCombineSourceNames[(int)to.cycle2Combine.alpha[i]]);
    }

    return result;
}

std::string generateSingleRenderMode(int renderMode, int cycleNumber) {
    std::ostringstream result;

    std::vector<std::string> flags;
    renderModeExtractFlags(renderMode, flags);

    for (auto& flag : flags) {
        if (result.tellp()) {
            result << " | ";
        }
        result << flag;
    }

    result << "GBL_c" << cycleNumber << "(";

    result << renderModeGetBlendModeName(renderMode, 0) << ", ";
    result << renderModeGetBlendModeName(renderMode, 1) << ", ";
    result << renderModeGetBlendModeName(renderMode, 2) << ", ";
    result << renderModeGetBlendModeName(renderMode, 3) << ")";

    return result.str();
}

std::unique_ptr<DataChunk> generateRenderMode(const MaterialState& from, const MaterialState& to) {
    if (!to.hasRenderMode ||
        (from.hasRenderMode && from.cycle1RenderMode == to.cycle1RenderMode && from.cycle2RenderMode == to.cycle2RenderMode)) {
        return NULL;
    }

    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsDPSetRenderMode"));

    std::string firstName;
    std::string secondName;

    for (auto pair : gRenderModes) {
        if (pair.second == to.cycle1RenderMode) {
            firstName = pair.first;
        }

        if (pair.second == to.cycle2RenderMode) {
            secondName = pair.first;
        }
    }

    if (firstName.length()) {
        result->AddPrimitive(firstName);
    } else {
        result->AddPrimitive(generateSingleRenderMode(to.cycle1RenderMode.data, 1));
    }

    if (secondName.length()) {
        result->AddPrimitive(secondName + "2");
    } else {
        result->AddPrimitive(generateSingleRenderMode(to.cycle1RenderMode.data, 2));
    }

    return result;
}

void generatePrimitiveColor(const MaterialState& from, const MaterialState& to, StructureDataChunk& output) {
    if (!to.usePrimitiveColor ||
        (from.usePrimitiveColor && from.primitiveColor == to.primitiveColor && from.primitiveL == to.primitiveL && from.primitiveM == to.primitiveM)) {
        return;   
    }

    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsDPSetPrimColor"));

    result->AddPrimitive((int)to.primitiveM);
    result->AddPrimitive((int)to.primitiveL);
    result->AddPrimitive((int)to.primitiveColor.r);
    result->AddPrimitive((int)to.primitiveColor.g);
    result->AddPrimitive((int)to.primitiveColor.b);
    result->AddPrimitive((int)to.primitiveColor.a);

    output.Add(std::move(result));
}

void generateColor(const PixelRGBAu8& to, const char* macroName, StructureDataChunk& output) {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk(macroName));

    result->AddPrimitive((int)to.r);
    result->AddPrimitive((int)to.g);
    result->AddPrimitive((int)to.b);
    result->AddPrimitive((int)to.a);

    output.Add(std::move(result));
}

std::string buildClampAndWrap(bool wrap, bool mirror) {
    std::ostringstream result;

    if (wrap) {
        result << "G_TX_WRAP";
    } else {
        result << "G_TX_CLAMP";
    }

    result << " | ";

    if (mirror) {
        result << "G_TX_MIRROR";
    } else {
        result << "G_TX_NOMIRROR";
    }

    return result.str();
}

void generateTile(CFileDefinition& fileDef, const MaterialState& from, const TileState& to, int tileIndex, StructureDataChunk& output) {
    if (!to.isOn) {
        return;
    }

    bool needsToLoadImage = to.texture != nullptr;

    for (int i = 0; i < MAX_TILE_COUNT && needsToLoadImage; ++i) {
        if (from.tiles[i].texture == to.texture && from.tiles[i].tmem == to.tmem) {
            needsToLoadImage = false;
        }
    }

    if (needsToLoadImage) {
        std::string imageName;
        if (!fileDef.GetResourceName(to.texture.get(), imageName)) {
            std::cerr << "Texture " << to.texture->Name() << " needs to be added to the file definition before being used in a material" << std::endl;
            return;
        }

        output.Add(std::unique_ptr<MacroDataChunk>(new MacroDataChunk("gsDPTileSync")));

        std::unique_ptr<MacroDataChunk> setTextureImage(new MacroDataChunk("gsDPSetTextureImage"));
        setTextureImage->AddPrimitive(nameForImageFormat(to.format));
        setTextureImage->AddPrimitive(std::string(nameForImageSize(to.size)) + "_LOAD_BLOCK");
        setTextureImage->AddPrimitive(1);
        setTextureImage->AddPrimitive(imageName);
        output.Add(std::move(setTextureImage));

        std::unique_ptr<MacroDataChunk> setTile(new MacroDataChunk("gsDPSetTile"));
        setTile->AddPrimitive(nameForImageFormat(to.format));
        setTile->AddPrimitive(std::string(nameForImageSize(to.size)) + "_LOAD_BLOCK");
        setTile->AddPrimitive(0);
        setTile->AddPrimitive(to.tmem);
        setTile->AddPrimitive<const char*>("G_TX_LOADTILE");
        setTile->AddPrimitive(to.pallete);
        setTile->AddPrimitive(buildClampAndWrap(to.tCoord.wrap, to.tCoord.mirror));
        setTile->AddPrimitive(to.tCoord.mask);
        setTile->AddPrimitive(to.tCoord.shift);
        setTile->AddPrimitive(buildClampAndWrap(to.sCoord.wrap, to.sCoord.mirror));
        setTile->AddPrimitive(to.sCoord.mask);
        setTile->AddPrimitive(to.sCoord.shift);
        output.Add(std::move(setTile));

        output.Add(std::unique_ptr<MacroDataChunk>(new MacroDataChunk("gsDPLoadSync")));

        std::unique_ptr<MacroDataChunk> loadBlock(new MacroDataChunk("gsDPLoadBlock"));
        loadBlock->AddPrimitive<const char*>("G_TX_LOADTILE");
        loadBlock->AddPrimitive(0);
        loadBlock->AddPrimitive(0);
        loadBlock->AddPrimitive(to.texture->LoadBlockSize());
        loadBlock->AddPrimitive(to.texture->DTX());
        output.Add(std::move(loadBlock));

        output.Add(std::unique_ptr<MacroDataChunk>(new MacroDataChunk("gsDPPipeSync")));
    }

    if (!from.tiles[tileIndex].IsTileStateEqual(to)) {
        std::unique_ptr<MacroDataChunk> setTile(new MacroDataChunk("gsDPSetTile"));

        setTile->AddPrimitive(nameForImageFormat(to.format));
        setTile->AddPrimitive(nameForImageSize(to.size));

        setTile->AddPrimitive(to.line);
        setTile->AddPrimitive(to.tmem);
        setTile->AddPrimitive(tileIndex);
        setTile->AddPrimitive(to.pallete);

        setTile->AddPrimitive(buildClampAndWrap(to.tCoord.wrap, to.tCoord.mirror));
        setTile->AddPrimitive(to.tCoord.mask);
        setTile->AddPrimitive(to.tCoord.shift);

        setTile->AddPrimitive(buildClampAndWrap(to.sCoord.wrap, to.sCoord.mirror));
        setTile->AddPrimitive(to.sCoord.mask);
        setTile->AddPrimitive(to.sCoord.shift);
        
        output.Add(std::move(setTile));
    }

    if (!from.tiles[tileIndex].IsTileSizeEqual(to)) {
        std::unique_ptr<MacroDataChunk> setTileSize(new MacroDataChunk("gsDPSetTileSize"));

        setTileSize->AddPrimitive(tileIndex);

        setTileSize->AddPrimitive(to.sCoord.offset);
        setTileSize->AddPrimitive(to.tCoord.offset);

        setTileSize->AddPrimitive(to.sCoord.limit);
        setTileSize->AddPrimitive(to.tCoord.limit);

        output.Add(std::move(setTileSize));
    }
}

void generateTexture(const TextureState& from, const TextureState& to, StructureDataChunk& output) {
    if (!to.isOn) {
        return;
    }

    if (from.isOn && from.sc == to.sc && from.tc == to.tc && from.level == to.level && from.tile == to.tile) {
        return;
    }
    
    std::unique_ptr<MacroDataChunk> setTexture(new MacroDataChunk("gsSPTexture"));
    setTexture->AddPrimitive(to.sc);
    setTexture->AddPrimitive(to.tc);
    setTexture->AddPrimitive(to.level);
    setTexture->AddPrimitive(to.tile);
    setTexture->AddPrimitive<const char*>("G_ON");
    output.Add(std::move(setTexture));
}

void generateMaterial(CFileDefinition& fileDef, const MaterialState& from, const MaterialState& to, StructureDataChunk& output) {
    output.Add(std::unique_ptr<DataChunk>(new MacroDataChunk("gsDPPipeSync")));

    generateEnumMacro((int)from.pipelineMode, (int)to.pipelineMode, "gsDPPipelineMode", gPipelineModeNames, output);
    generateEnumMacro((int)from.cycleType, (int)to.cycleType, "gsDPSetCycleType", gCycleTypeNames, output);
    generateEnumMacro((int)from.perspectiveMode, (int)to.perspectiveMode, "gsDPSetTexturePersp", gPerspectiveModeNames, output);
    generateEnumMacro((int)from.textureDetail, (int)to.textureDetail, "gsDPSetTextureDetail", gTextureDetailNames, output);
    generateEnumMacro((int)from.textureLOD, (int)to.textureLOD, "gsDPSetTextureLOD", gTextureLODNames, output);
    generateEnumMacro((int)from.textureLUT, (int)to.textureLUT, "gsDPSetTextureLUT", gTextureLUTNames, output);
    generateEnumMacro((int)from.textureFilter, (int)to.textureFilter, "gsDPSetTextureFilter", gTextureFilterNames, output);
    generateEnumMacro((int)from.textureConvert, (int)to.textureConvert, "gsDPSetTextureConvert", gTextureConvertNames, output);
    generateEnumMacro((int)from.combineKey, (int)to.combineKey, "gsDPSetCombineKey", gCombineKeyNames, output);
    generateEnumMacro((int)from.colorDither, (int)to.colorDither, "gsDPSetColorDither", gCotherDitherNames, output);
    generateEnumMacro((int)from.alphaDither, (int)to.alphaDither, "gsDPSetAlphaDither", gAlphaDitherNames, output);
    generateEnumMacro((int)from.alphaCompare, (int)to.alphaCompare, "gsDPSetAlphaCompare", gAlphaCompareNames, output);
    generateEnumMacro((int)from.depthSource, (int)to.depthSource, "gsDPSetDepthSource", gDepthSourceNames, output);

    std::unique_ptr<DataChunk> geometryModes = std::move(generateGeometryModes(from, to));
    if (geometryModes) {
        output.Add(std::move(geometryModes));
    }

    std::unique_ptr<DataChunk> combineMode = std::move(generateCombineMode(from, to));
    if (combineMode) {
        output.Add(std::move(combineMode));
    }

    std::unique_ptr<DataChunk> renderMode = std::move(generateRenderMode(from, to));
    if (renderMode) {
        output.Add(std::move(renderMode));
    }

    generatePrimitiveColor(from, to, output);

    if (to.useEnvColor && (!from.useEnvColor || !(to.envColor == from.envColor))) {
        generateColor(to.envColor, "gsDPSetEnvColor", output);
    }

    if (to.useFogColor && (!from.useFogColor || !(to.fogColor == from.fogColor))) {
        generateColor(to.fogColor, "gsDPSetFogColor", output);
    }

    if (to.useBlendColor && (!from.useBlendColor || !(to.blendColor == from.blendColor))) {
        generateColor(to.blendColor, "gsDPSetBlendColor", output);
    }

    generateTexture(from.textureState, to.textureState, output);

    for (int i = 0; i < MAX_TILE_COUNT; ++i) {
        generateTile(fileDef, from, to.tiles[i], i, output);
    }

    // TODO fill color
}