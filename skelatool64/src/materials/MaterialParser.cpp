
#include "MaterialParser.h"

#include "yaml-cpp/yaml.h"
#include <algorithm>
#include <string.h>
#include <sstream>
#include <string>
#include <stdexcept>
#include <map>

#include "./TextureCache.h"
#include "../FileUtils.h"
#include "./RenderMode.h"
#include "CombineMode.h"

ParseError::ParseError(const std::string& message) :
    mMessage(message) {
    
}

ParseResult::ParseResult(const std::string& insideFolder) : mInsideFolder(insideFolder) {}

std::string formatError(const std::string& message, const YAML::Mark& mark) {
    std::stringstream output;
    output << "error at line " << mark.line + 1 << ", column "
           << mark.column + 1 << ": " << message;
    return output.str();
}

int parseInteger(const YAML::Node& node, ParseResult& output, int min, int max) {
    if (!node.IsDefined() || !node.IsScalar()) {
        output.mErrors.push_back(ParseError(formatError("Expected a number", node.Mark())));
        return 0;
    }
    
    int result = 0;

    try {
        result = std::atoi(node.Scalar().c_str());
    } catch (std::invalid_argument const& err) {
        output.mErrors.push_back(ParseError(formatError("Expected a number", node.Mark())));
        return 0;
    }

    if (result < min || result > max) {
        std::stringstream errorMessage;
        errorMessage << "Expected a number between " << min << " and " << max;
        output.mErrors.push_back(ParseError(formatError(errorMessage.str(), node.Mark())));
        return result;
    }


    return result;
}

int parseOptionalInteger(const YAML::Node& node, ParseResult& output, int min, int max, int defaultValue) {
    if (!node.IsDefined()) {
        return defaultValue;
    }

    return parseInteger(node, output, min, max);
}

std::string parseString(const YAML::Node& node, ParseResult& output) {
    if (!node.IsDefined() || !node.IsScalar()) {
        output.mErrors.push_back(ParseError(formatError("Expected a string", node.Mark())));
        return "";
    }

    return node.as<std::string>();
}

template <typename T>
T parseEnumType(const YAML::Node& node, ParseResult& output, const char** names, T defaultValue, int count) {
    if (!node.IsDefined()) {
        return defaultValue;
    }

    if (!node.IsScalar()) {
        output.mErrors.push_back(ParseError(formatError("Invalid type for enum", node.Mark())));
        return defaultValue;
    }

    std::string asString = node.as<std::string>();

    for (int i = 0; i < count; ++i) {
        if (asString == names[i]) {
            return (T)i;
        }
    }

    output.mErrors.push_back(ParseError(formatError("Invalid type for enum", node.Mark())));
    return defaultValue;
}

bool parseMaterialColor(const YAML::Node& node, PixelRGBAu8& color, ParseResult& output) {
    if (!node.IsDefined()) {
        return false;
    }

    if (!node.IsMap()) {
        output.mErrors.push_back(ParseError(formatError("Color is expected to be map with r,g,b", node.Mark())));
        return false;
    }

    color.r = parseInteger(node["r"], output, 0, 255);
    color.g = parseInteger(node["g"], output, 0, 255);
    color.b = parseInteger(node["b"], output, 0, 255);
    color.a = parseOptionalInteger(node["a"], output, 0, 255, 255);

    auto bypassNode = node["bypassEffects"];
    
    bool bypass = bypassNode.IsDefined() && bypassNode.as<bool>();

    if (!bypass && output.mForcePallete.length() && output.mTargetCIBuffer) {
        std::shared_ptr<PalleteDefinition> pallete = gTextureCache.GetPallete(output.mForcePallete);

        PixelIu8 colorindex = pallete->FindIndex(color);

        color.r = colorindex.i;
        color.g = colorindex.i;
        color.b = colorindex.i;
    }

    return true;
}

void parsePrimColor(const YAML::Node& node, MaterialState& state, ParseResult& output) {
    bool result = parseMaterialColor(node, state.primitiveColor, output);

    if (!result) {
        return;
    }

    state.usePrimitiveColor = true;

    YAML::Node m = node["m"];
    if (m.IsDefined()) {
        state.primitiveM = parseInteger(m, output, 0, 255);
    }
    
    YAML::Node l = node["l"];
    if (l.IsDefined()) {
        state.primitiveL = parseInteger(l, output, 0, 255);
    }
} 

NormalSource parseMaterialNormalSource(const YAML::Node& node) {
    if (node.IsDefined() && node.IsScalar()) {
        std::string name = node.Scalar();

        if (name == "normal") {
            return NormalSource::Normal;
        }
        if (name == "tangent") {
            return NormalSource::Tangent;
        }
        if (name == "-tangent") {
            return NormalSource::MinusTangent;
        }
        if (name == "bitangent") {
            return NormalSource::Bitangent;
        }
        if (name == "-bitangent") {
            return NormalSource::MinusCotangent;
        }
    }

    return NormalSource::Normal;
}

G_IM_FMT parseTextureFormat(const YAML::Node& node, ParseResult& output) {
    std::string asString = parseString(node, output);

    if (asString == "G_IM_FMT_RGBA") {
        return G_IM_FMT::G_IM_FMT_RGBA;
    }

    if (asString == "G_IM_FMT_RGBA") {
        return G_IM_FMT::G_IM_FMT_YUV;
    }

    if (asString == "G_IM_FMT_CI") {
        return G_IM_FMT::G_IM_FMT_CI;
    }

    if (asString == "G_IM_FMT_I") {
        return G_IM_FMT::G_IM_FMT_I;
    }

    if (asString == "G_IM_FMT_IA") {
        return G_IM_FMT::G_IM_FMT_IA;
    }

    output.mErrors.push_back(ParseError(formatError("Texture format should be G_IM_FMT_RGBA, G_IM_FMT_YUV, G_IM_FMT_CI, G_IM_FMT_I, or G_IM_FMT_IA", node.Mark())));

    return G_IM_FMT::G_IM_FMT_RGBA;
}

G_IM_SIZ parseTextureSize(const YAML::Node& node, ParseResult& output) {
    std::string asString = parseString(node, output);

    if (asString == "G_IM_SIZ_32b") {
        return G_IM_SIZ::G_IM_SIZ_32b;
    }

    if (asString == "G_IM_SIZ_16b") {
        return G_IM_SIZ::G_IM_SIZ_16b;
    }

    if (asString == "G_IM_SIZ_8b") {
        return G_IM_SIZ::G_IM_SIZ_8b;
    }

    if (asString == "G_IM_SIZ_4b") {
        return G_IM_SIZ::G_IM_SIZ_4b;
    }

    output.mErrors.push_back(ParseError(formatError("Texture size should be G_IM_SIZ_32b, G_IM_SIZ_16b, G_IM_SIZ_8b, or G_IM_SIZ_4b", node.Mark())));

    return G_IM_SIZ::G_IM_SIZ_16b;
}

G_IM_SIZ gDefaultImageSize[] = {
    // G_IM_FMT_RGBA
    G_IM_SIZ::G_IM_SIZ_16b,
    // G_IM_FMT_YUV
    G_IM_SIZ::G_IM_SIZ_16b,
    // G_IM_FMT_CI
    G_IM_SIZ::G_IM_SIZ_8b,
    // G_IM_FMT_I
    G_IM_SIZ::G_IM_SIZ_8b,
    // G_IM_FMT_IA
    G_IM_SIZ::G_IM_SIZ_16b,
};

std::shared_ptr<TextureDefinition> parseTextureDefinition(const YAML::Node& node, ParseResult& output) {
    if (!node.IsDefined()) {
        return NULL;
    }

    std::string filename;
    std::string palleteFilename = output.mForcePallete;

    bool hasFormat = false;
    G_IM_FMT requestedFormat;
    bool hasSize = false;
    G_IM_SIZ requestedSize;

    TextureDefinitionEffect effects = (TextureDefinitionEffect)0;

    if (node.IsScalar()) {
        filename = parseString(node, output);
    } else if (node.IsMap()) {
        filename = parseString(node["filename"], output);

        auto yamlFormat = node["fmt"];
        if (yamlFormat.IsDefined()) {
            requestedFormat = parseTextureFormat(yamlFormat, output);
            hasFormat = true;
        }

        auto yamlSize = node["siz"];
        if (yamlSize.IsDefined()) {
            requestedSize = parseTextureSize(yamlSize, output);
            hasSize = true;
        }

        auto twoTone = node["twoTone"];
        if (twoTone.IsDefined() && twoTone.as<bool>()) {
            effects = (TextureDefinitionEffect)((int)effects | (int)TextureDefinitionEffect::TwoToneGrayscale);

            if (!yamlFormat.IsDefined()) {
                requestedFormat = G_IM_FMT::G_IM_FMT_I;
                hasFormat = true;
            }
        }

        auto normalMap = node["normalMap"];
        if (normalMap.IsDefined() && normalMap.as<bool>()) {
            effects = (TextureDefinitionEffect)((int)effects | (int)TextureDefinitionEffect::NormalMap);
        }

        auto invert = node["invert"];
        if (invert.IsDefined() && invert.as<bool>()) {
            effects = (TextureDefinitionEffect)((int)effects | (int)TextureDefinitionEffect::Invert);
        }

        auto selectChannel = node["selectChannel"];
        if (selectChannel.IsDefined()) {
            auto channel = selectChannel.as<std::string>();

            if (channel == "r") {
                effects = (TextureDefinitionEffect)((int)effects | (int)TextureDefinitionEffect::SelectR);
            } else if (channel == "g") {
                effects = (TextureDefinitionEffect)((int)effects | (int)TextureDefinitionEffect::SelectG);
            } else if (channel == "b") {
                effects = (TextureDefinitionEffect)((int)effects | (int)TextureDefinitionEffect::SelectB);
            }
        }

        auto usePallete = node["usePallete"];

        if (usePallete.IsDefined()) {
            if (usePallete.IsScalar()) {
                palleteFilename = usePallete.as<std::string>();
            } else {
                output.mErrors.push_back(ParseError(formatError(std::string("usePallete should be a file path to a pallete") + filename, usePallete.Mark())));
            }
        }
    } else {
        output.mErrors.push_back(ParseError(formatError(std::string("Tile should be a file name or object") + filename, node.Mark())));
        return NULL;
    }

    filename = Join(output.mInsideFolder, filename);

    if (!FileExists(filename)) {
        output.mErrors.push_back(ParseError(formatError(std::string("Could not open file ") + filename, node.Mark())));
        return NULL;
    }

    G_IM_FMT format;
    G_IM_SIZ size;

    if (hasFormat && hasSize) {
        format = requestedFormat;
        size = requestedSize;
    } else {
        TextureDefinition::DetermineIdealFormat(filename, format, size);

        if (hasFormat) {
            if (format != requestedFormat) {
                size = gDefaultImageSize[(int)requestedFormat];
            }
            format = requestedFormat;
        }

        if (hasSize) {
            size = requestedSize;
        }
    }

    if (!isImageFormatSupported(format, size)) {
        output.mErrors.push_back(ParseError(formatError("Unsupported image format ", node.Mark())));
        return NULL;
    }

    return gTextureCache.GetTexture(filename, format, size, effects, palleteFilename);
}

int parseRenderModeFlags(const YAML::Node& node, ParseResult& output) {
    if (!node.IsDefined()) {
        return 0;
    }

    if (!node.IsSequence()) {
        output.mErrors.push_back(ParseError(formatError("Render mode flags should be an array of strings", node.Mark())));
        return 0;
    }

    int result = 0;

    for (unsigned i = 0; i < node.size(); ++i) {
        const YAML::Node& element = node[i];

        if (!element.IsScalar()) {
            output.mErrors.push_back(ParseError(formatError("Flags should be a list of strings", element.Mark())));
            continue;
        }

        std::string asString = element.as<std::string>();

        int singleFlag = 0;

        if (!renderModeGetFlagValue(asString, singleFlag)) {    
            output.mErrors.push_back(ParseError(formatError("Invalid flag", element.Mark())));
            continue;
        }

        result |= singleFlag;
    }

    return result;
}

int parseBlendMode(const YAML::Node& node, ParseResult& output) {
    if (!node.IsDefined()) {
        return 0;
    }

    if (!node.IsSequence() || node.size() != 4) {
        output.mErrors.push_back(ParseError(formatError("Render blend mode should be an array of 4 strings", node.Mark())));
        return 0;
    }

    int params[4];

    for (int i = 0; i < 4; ++i) {
        const YAML::Node& element = node[i];
        params[i] = 0;

        if (!element.IsScalar()) {
            output.mErrors.push_back(ParseError(formatError("Expected a string", node.Mark())));
            continue;
        }

        std::string asString = element.as<std::string>();

        if (!renderModeGetBlendModeValue(asString, i, params[i])) {
            output.mErrors.push_back(ParseError(formatError(std::string("Invalid blend mode ") + asString, node.Mark())));
        }
    }

    return GBL_c1(params[0], params[1], params[2], params[3]);
}

void parseSingleRenderMode(const YAML::Node& node, RenderModeState& renderMode, ParseResult& output) {
    if (node.IsScalar()) {
        std::string asString = node.as<std::string>();

        if (!findRenderModeByName(asString, renderMode)) {
            output.mErrors.push_back(ParseError(formatError("Invalid render mode", node.Mark())));
            return;
        }
        
        return;
    }

    if (node.IsMap()) {
        renderMode.data = 
            parseRenderModeFlags(node["flags"], output) |
            parseBlendMode(node["blend"], output);
        return;
    }

    output.mErrors.push_back(ParseError(formatError("Invalid render mode", node.Mark())));
}

void parseRenderMode(const YAML::Node& node, MaterialState& state, ParseResult& output) {
    if (!node.IsDefined()) {
        state.hasRenderMode = false;
        return;
    }
    state.hasRenderMode = true;

    if (node.IsScalar() || node.IsMap()) {
        parseSingleRenderMode(node, state.cycle1RenderMode, output);
        state.cycle2RenderMode = state.cycle1RenderMode;
        return;
    }

    if (node.IsSequence() && node.size() == 2) {
        parseSingleRenderMode(node[0], state.cycle1RenderMode, output);
        parseSingleRenderMode(node[1], state.cycle2RenderMode, output);
        return;
    }

    output.mErrors.push_back(ParseError(formatError("Invalid render mode", node.Mark())));
}

void parseColorCombineMode(const YAML::Node& node, ColorCombineMode& combineMode, ParseResult& output) {
    if (!node.IsDefined()) {
        combineMode.color[0] = ColorCombineSource::_0;
        combineMode.color[1] = ColorCombineSource::_0;
        combineMode.color[2] = ColorCombineSource::_0;
        combineMode.color[3] = ColorCombineSource::_0;
        return;
    }

    if (!node.IsSequence() || node.size() != 4) {
        output.mErrors.push_back(ParseError(formatError("Blend mode should be an array of strings", node.Mark())));
        return;
    }

    for (int i = 0; i < 4; ++i) {
        combineMode.color[i] = parseEnumType(node[i], output, gColorCombineSourceNames, ColorCombineSource::_0, (int)ColorCombineSource::Count);
    }
}

void parseAlphaCombineMode(const YAML::Node& node, ColorCombineMode& combineMode, ParseResult& output) {
    if (!node.IsDefined()) {
        combineMode.alpha[0] = AlphaCombineSource::_0;
        combineMode.alpha[1] = AlphaCombineSource::_0;
        combineMode.alpha[2] = AlphaCombineSource::_0;
        combineMode.alpha[3] = AlphaCombineSource::_1;
        return;
    }

    if (!node.IsSequence() || node.size() != 4) {
        output.mErrors.push_back(ParseError(formatError("Blend mode should be an array of strings", node.Mark())));
        return;
    }

    for (int i = 0; i < 4; ++i) {
        combineMode.alpha[i] = parseEnumType(node[i], output, gAlphaCombineSourceNames, AlphaCombineSource::_0, (int)AlphaCombineSource::Count);
    }
}

void parseSingleCombineMode(const YAML::Node& node, ColorCombineMode& combineMode, ParseResult& output) {
    if (!node.IsDefined()) {
        output.mErrors.push_back(ParseError(formatError("Combine mode should be an object with a color and alpha", node.Mark())));
        return;
    }
    
    if (node.IsMap()) {
        parseColorCombineMode(node["color"], combineMode, output);
        parseAlphaCombineMode(node["alpha"], combineMode, output);
        return;
    }

    if (node.IsScalar()) {
        std::string name = node.as<std::string>();
        if (!combineModeWithName(name, combineMode)) {
            output.mErrors.push_back(ParseError(formatError(name + " is not a valid name for a combine mode", node.Mark())));
        }
        return;
    }

    output.mErrors.push_back(ParseError(formatError("Combine mode should be an object with a color and alpha", node.Mark())));
}

void parseCombineMode(const YAML::Node& node, MaterialState& state, ParseResult& output) {
    if (!node.IsDefined()) {
        return;
    }
    state.hasCombineMode = true;

    if (node.IsMap() || node.IsScalar()) {
        parseSingleCombineMode(node, state.cycle1Combine, output);
        state.cycle2Combine = state.cycle1Combine;
        return;
    }    

    if (node.IsSequence() && node.size() == 2) {
        parseSingleCombineMode(node[0], state.cycle1Combine, output);
        parseSingleCombineMode(node[1], state.cycle2Combine, output);
        return;
    }

    output.mErrors.push_back(ParseError(formatError("Combine mode should be a map with a color and alpha", node.Mark())));
}

void parseGeometryModeSequence(const YAML::Node& node, FlagList& result, bool flagValue, ParseResult& output) {
    if (!node.IsDefined()) {
        return;
    }

    if (!node.IsSequence()) {
        output.mErrors.push_back(ParseError(formatError("Should be a string array", node.Mark())));
        return;
    }

    for (std::size_t i = 0; i < node.size(); ++i) {
        const YAML::Node& element = node[i];

        if (!element.IsScalar()) {
            output.mErrors.push_back(ParseError(formatError("Expected a string", element.Mark())));
            continue;
        }

        int mask = 1 << parseEnumType(element, output, gGeometryModeNames, 0, GEOMETRY_MODE_COUNT);

        result.SetFlag(mask, flagValue);
    }
}

FlagList parseGeometryMode(const YAML::Node& node, ParseResult& output) {
    FlagList result;

    if (!node.IsDefined()) {
        return result;
    }

    parseGeometryModeSequence(node["clear"], result, false, output);
    parseGeometryModeSequence(node["set"], result, true, output);

    return result;
}

void parseTexture(const YAML::Node& node, TextureState& state, ParseResult& output) {
    if (!node.IsDefined()) {
        return;
    }

    if (!node.IsMap()) {
        output.mErrors.push_back(ParseError(formatError("gSPTexture should be a map", node.Mark())));
    }

    state.sc = parseOptionalInteger(node["sc"], output, 0, 0xFFFF, 0xFFFF);
    state.tc = parseOptionalInteger(node["tc"], output, 0, 0xFFFF, 0xFFFF);
    state.level = parseOptionalInteger(node["level"], output, 0, 7, 0);
    state.tile = parseOptionalInteger(node["tile"], output, 0, 7, 0);

    state.isOn = true;
}

bool isEvenLog2(int size) {
    return ((size - 1) & size) == 0;
}

int log2(int size) {
    int result = 0;

    --size;

    while (size) {
        ++result;
        size >>= 1;
    }

    return result;
}

void parseTextureCoordinate(const YAML::Node& node, TextureCoordinateState& state, ParseResult& output) {
    if (!node.IsDefined()) {
        return;
    }

    auto wrap = node["wrap"];
    if (wrap.IsDefined()) {
        state.wrap = wrap.as<bool>();
    }

    auto mirror = node["mirror"];
    if (mirror.IsDefined()) {
        state.mirror = mirror.as<bool>();
    }

    auto mask = node["mask"];
    if (mask.IsDefined()) {
        state.mask = mask.as<int>();
    }

    auto shift = node["shift"];
    if (shift.IsDefined()) {
        state.shift = shift.as<int>();
    }

    auto offset = node["offset"];
    if (offset.IsDefined()) {
        state.offset = offset.as<int>();
    }

    auto limit = node["limit"];
    if (limit.IsDefined()) {
        state.limit = limit.as<int>();
    }
}

bool parseSingleTile(const YAML::Node& node, TileState& state, ParseResult& output) {
    state.texture = parseTextureDefinition(node, output);

    state.isOn = node.IsDefined();

    if (state.texture) {
        state.format = state.texture->Format();
        state.size = state.texture->Size();
        if (!state.texture->GetLine(state.line)) {
            output.mErrors.push_back(ParseError(formatError("Texture line width should be a multiple of 64 bits", node.Mark())));
        }

        state.sCoord.mask = log2(state.texture->Width());
        state.tCoord.mask = log2(state.texture->Height());

        state.sCoord.limit = (state.texture->Width() - 1) * 4;
        state.tCoord.limit = (state.texture->Height() - 1) * 4;
    }

    if (node.IsMap()) {
        auto yamlFormat = node["fmt"];
        if (!state.texture && yamlFormat.IsDefined()) {
            state.format = parseTextureFormat(yamlFormat, output);
        }

        auto yamlSize = node["siz"];
        if (!state.texture && yamlSize.IsDefined()) {
            state.size = parseTextureSize(yamlSize, output);
        }

        state.tmem = parseOptionalInteger(node["tmem"], output, 0, 511, 0);
        state.pallete = parseOptionalInteger(node["pallete"], output, 0, 15, 0);
    }
    
    parseTextureCoordinate(node["s"], state.sCoord, output);
    parseTextureCoordinate(node["t"], state.tCoord, output);

    return state.isOn;
}

void parseTiles(const YAML::Node& node, MaterialState& state, ParseResult& output) {
    if (!node.IsDefined()) {
        return;
    }

    if (node.IsMap() || node.IsScalar()) {
        if (parseSingleTile(node, state.tiles[0], output) || state.textureState.isOn) {
            state.textureState.isOn = true;
        }
        return;
    }

    if (node.IsSequence()) {
        if (node.size() > 8) {
            output.mErrors.push_back(ParseError(formatError("Only up to 8 tiles are supported", node.Mark())));
        }

        for (std::size_t i = 0; i < node.size() && i < 8; ++i) {
            const YAML::Node& element = node[i];

            if (!element.IsNull() && parseSingleTile(element, state.tiles[i], output)) {
                state.textureState.isOn = true;
            }
        }
        return;
    }

    output.mErrors.push_back(ParseError(formatError("Expected a tile or array of tiles", node.Mark())));
}

ColorCombineMode gTwoToneCombineMode(
    ColorCombineSource::PrimitiveColor, 
    ColorCombineSource::EnvironmentColor, 
    ColorCombineSource::Texel0, 
    ColorCombineSource::EnvironmentColor,

    AlphaCombineSource::_0,
    AlphaCombineSource::_0,
    AlphaCombineSource::_0,
    AlphaCombineSource::Texture0Alpha
);
 
std::shared_ptr<Material> parseMaterial(const std::string& name, const YAML::Node& node, ParseResult& output) {
    std::shared_ptr<Material> material(new Material(name));

    parseTexture(node["gSPTexture"], material->mState.textureState, output);

    parseTiles(node["gDPSetTile"], material->mState, output);

    for (int i = 0; i < MAX_TILE_COUNT; ++i) {
        if (material->mState.tiles[i].texture && material->mState.tiles[i].texture->HasEffect(TextureDefinitionEffect::TwoToneGrayscale)) {
            material->mState.envColor = material->mState.tiles[i].texture->GetTwoToneMin();
            material->mState.useEnvColor = true;
            material->mState.primitiveColor = material->mState.tiles[i].texture->GetTwoToneMax();
            material->mState.usePrimitiveColor = true;

            material->mState.cycle1Combine = gTwoToneCombineMode;
            material->mState.cycle2Combine = gTwoToneCombineMode;
            material->mState.hasCombineMode = true;

            break;
        }
    }

    parseRenderMode(node["gDPSetRenderMode"], material->mState, output);
    parseCombineMode(node["gDPSetCombineMode"], material->mState, output);

    material->mState.geometryModes = parseGeometryMode(node["gSPGeometryMode"], output);

    material->mState.pipelineMode = parseEnumType(node["gDPPipelineMode"], output, gPipelineModeNames, PipelineMode::Unknown, (int)PipelineMode::Count);
    material->mState.cycleType = parseEnumType(node["gDPSetCycleType"], output, gCycleTypeNames, CycleType::Unknown, (int)CycleType::Count);
    material->mState.perspectiveMode = parseEnumType(node["gDPSetTexturePersp"], output, gPerspectiveModeNames, PerspectiveMode::Unknown, (int)PerspectiveMode::Count);
    material->mState.textureDetail = parseEnumType(node["gDPSetTextureDetail"], output, gTextureDetailNames, TextureDetail::Unknown, (int)TextureDetail::Count);
    material->mState.textureLOD = parseEnumType(node["gDPSetTextureLOD"], output, gTextureLODNames, TextureLOD::Unknown, (int)TextureLOD::Count);
    material->mState.textureLUT = parseEnumType(node["gDPSetTextureLUT"], output, gTextureLUTNames, TextureLUT::Unknown, (int)TextureLUT::Count);
    material->mState.textureFilter = parseEnumType(node["gDPSetTextureFilter"], output, gTextureFilterNames, TextureFilter::Unknown, (int)TextureFilter::Count);
    material->mState.textureConvert = parseEnumType(node["gDPSetTextureConvert"], output, gTextureConvertNames, TextureConvert::Unknown, (int)TextureConvert::Count);
    material->mState.combineKey = parseEnumType(node["gDPSetCombineKey"], output, gCombineKeyNames, CombineKey::Unknown, (int)CombineKey::Count);
    material->mState.colorDither = parseEnumType(node["gDPSetColorDither"], output, gCotherDitherNames, ColorDither::Unknown, (int)ColorDither::Count);
    material->mState.alphaDither = parseEnumType(node["gDPSetAlphaDither"], output, gAlphaDitherNames, AlphaDither::Unknown, (int)AlphaDither::Count);
    material->mState.alphaCompare = parseEnumType(node["gDPSetAlphaCompare"], output, gAlphaCompareNames, AlphaCompare::Unknown, (int)AlphaCompare::Count);
    material->mState.depthSource = parseEnumType(node["gDPSetDepthSource"], output, gDepthSourceNames, DepthSource::Unknown, (int)DepthSource::Count);
    
    parsePrimColor(node["gDPSetPrimColor"], material->mState, output);
    material->mState.useEnvColor = parseMaterialColor(node["gDPSetEnvColor"], material->mState.envColor, output) || material->mState.useEnvColor;
    material->mState.useFogColor = parseMaterialColor(node["gDPSetFogColor"], material->mState.fogColor, output) || material->mState.useFogColor;
    material->mState.useBlendColor = parseMaterialColor(node["gDPSetBlendColor"], material->mState.blendColor, output) || material->mState.useBlendColor;

    material->mNormalSource = parseMaterialNormalSource(node["normalSource"]);

    auto excludeFromOutput = node["excludeFromOutput"];

    material->mExcludeFromOutut = excludeFromOutput.IsDefined() && excludeFromOutput.as<bool>();

    auto properties = node["properties"];

    if (properties.IsDefined() && properties.IsMap()) {
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            material->mProperties[it->first.as<std::string>()] = it->second.as<std::string>();
        }
    }

    return material;

}

void parseMaterialFile(std::istream& input, ParseResult& output) {
    try {
        YAML::Node doc = YAML::Load(input);

        const YAML::Node& materials = doc["materials"];

        for (auto it = materials.begin(); it != materials.end(); ++it) {
            std::string name = it->first.as<std::string>();
            output.mMaterialFile.mMaterials[name] = parseMaterial(name, it->second, output);
        }
    } catch (YAML::ParserException& e) {
        output.mErrors.push_back(ParseError(e.what()));
    }
}