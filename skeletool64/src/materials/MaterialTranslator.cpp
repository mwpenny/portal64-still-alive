#include "MaterialTranslator.h"

#include "MaterialEnums.h"

void loadTextureFromAiMaterial(TextureCache& cache, Material& material, aiString filename, DisplayListSettings& settings) {
    G_IM_FMT fmt;
    G_IM_SIZ siz;
    TextureDefinition::DetermineIdealFormat(filename.C_Str(), fmt, siz);
    
    std::shared_ptr<TextureDefinition> useTexture = cache.GetTexture(filename.C_Str(), fmt, siz, (TextureDefinitionEffect)0, settings.mForcePallete);

    material.mState.textureState.isOn = 1;
    material.mState.textureState.level = 0;
    material.mState.textureState.sc = 0xffff;
    material.mState.textureState.tc = 0xffff;
    material.mState.textureState.tile = 0;

    TileState& state = material.mState.tiles[0];

    state.isOn = true;
    state.texture = useTexture;

    if (!state.texture->GetLineForTile(state.line)) {
        std::cerr << "Texture line width should be a multiple of 64 bits" << std::endl;
    }
    
    state.format = useTexture->Format();
    state.size = useTexture->Size();
    state.tmem = 0;

    state.sCoord.mask = log2(state.texture->Width());
    state.tCoord.mask = log2(state.texture->Height());

    state.sCoord.limit = (state.texture->Width() - 1) * 4;
    state.tCoord.limit = (state.texture->Height() - 1) * 4;

    state.sCoord.wrap = true;
    state.tCoord.wrap = true;

    state.sCoord.mirror = false;
    state.tCoord.mirror = false;
}

#define TEXTURE_LIT "texture_lit"
#define TEXTURE_UNLIT "texture_unlit"
#define SOLID_LIT "solid_lit"
#define SOLID_UNLIT "solid_unlit"

void fillDefaultMaterials(DisplayListSettings& settings) {
    if (settings.mMaterials.find(TEXTURE_LIT) == settings.mMaterials.end()) {
        std::shared_ptr<Material> textureLit(new Material(TEXTURE_LIT));

        textureLit->mState.hasCombineMode = true;
        textureLit->mState.cycle1Combine.color[0] = ColorCombineSource::ShadeColor;
        textureLit->mState.cycle1Combine.color[1] = ColorCombineSource::_0;
        textureLit->mState.cycle1Combine.color[2] = ColorCombineSource::Texel0;
        textureLit->mState.cycle1Combine.color[3] = ColorCombineSource::_0;

        textureLit->mState.cycle1Combine.alpha[0] = AlphaCombineSource::_0;
        textureLit->mState.cycle1Combine.alpha[1] = AlphaCombineSource::_0;
        textureLit->mState.cycle1Combine.alpha[2] = AlphaCombineSource::_0;
        textureLit->mState.cycle1Combine.alpha[3] = AlphaCombineSource::Texture0Alpha;

        textureLit->mState.cycle2Combine = textureLit->mState.cycle1Combine;

        textureLit->mState.geometryModes.SetFlag((int)GeometryMode::G_LIGHTING, true);
        textureLit->mState.geometryModes.SetFlag((int)GeometryMode::G_SHADE, true);

        textureLit->mExcludeFromOutut = true;

        settings.mMaterials[TEXTURE_LIT] = textureLit;
    }
    
    if (settings.mMaterials.find(TEXTURE_UNLIT) == settings.mMaterials.end()) {
        std::shared_ptr<Material> textureUnlit(new Material(TEXTURE_LIT));

        textureUnlit->mState.hasCombineMode = true;
        textureUnlit->mState.cycle1Combine.color[0] = ColorCombineSource::_0;
        textureUnlit->mState.cycle1Combine.color[1] = ColorCombineSource::_0;
        textureUnlit->mState.cycle1Combine.color[2] = ColorCombineSource::_0;
        textureUnlit->mState.cycle1Combine.color[3] = ColorCombineSource::Texel0;

        textureUnlit->mState.cycle1Combine.alpha[0] = AlphaCombineSource::_0;
        textureUnlit->mState.cycle1Combine.alpha[1] = AlphaCombineSource::_0;
        textureUnlit->mState.cycle1Combine.alpha[2] = AlphaCombineSource::_0;
        textureUnlit->mState.cycle1Combine.alpha[3] = AlphaCombineSource::Texture0Alpha;

        textureUnlit->mState.cycle2Combine = textureUnlit->mState.cycle1Combine;

        textureUnlit->mState.geometryModes.SetFlag((int)GeometryMode::G_LIGHTING, false);
        textureUnlit->mState.geometryModes.SetFlag((int)GeometryMode::G_SHADE, true);

        textureUnlit->mExcludeFromOutut = true;

        settings.mMaterials[TEXTURE_UNLIT] = textureUnlit;
    }

    if (settings.mMaterials.find(SOLID_LIT) == settings.mMaterials.end()) {
        std::shared_ptr<Material> solidLit(new Material(SOLID_LIT));

        solidLit->mState.hasCombineMode = true;
        solidLit->mState.cycle1Combine.color[0] = ColorCombineSource::ShadeColor;
        solidLit->mState.cycle1Combine.color[1] = ColorCombineSource::_0;
        solidLit->mState.cycle1Combine.color[2] = ColorCombineSource::PrimitiveColor;
        solidLit->mState.cycle1Combine.color[3] = ColorCombineSource::_0;

        solidLit->mState.cycle1Combine.alpha[0] = AlphaCombineSource::_0;
        solidLit->mState.cycle1Combine.alpha[1] = AlphaCombineSource::_0;
        solidLit->mState.cycle1Combine.alpha[2] = AlphaCombineSource::_0;
        solidLit->mState.cycle1Combine.alpha[3] = AlphaCombineSource::PrimitiveAlpha;

        solidLit->mState.cycle2Combine = solidLit->mState.cycle1Combine;

        solidLit->mState.geometryModes.SetFlag((int)GeometryMode::G_LIGHTING, true);
        solidLit->mState.geometryModes.SetFlag((int)GeometryMode::G_SHADE, true);

        solidLit->mExcludeFromOutut = true;

        settings.mMaterials[SOLID_LIT] = solidLit;
    }

    if (settings.mMaterials.find(SOLID_UNLIT) == settings.mMaterials.end()) {
        std::shared_ptr<Material> solidUnlit(new Material(SOLID_UNLIT));

        solidUnlit->mState.hasCombineMode = true;
        solidUnlit->mState.cycle1Combine.color[0] = ColorCombineSource::_0;
        solidUnlit->mState.cycle1Combine.color[1] = ColorCombineSource::_0;
        solidUnlit->mState.cycle1Combine.color[2] = ColorCombineSource::_0;
        solidUnlit->mState.cycle1Combine.color[3] = ColorCombineSource::PrimitiveColor;

        solidUnlit->mState.cycle1Combine.alpha[0] = AlphaCombineSource::_0;
        solidUnlit->mState.cycle1Combine.alpha[1] = AlphaCombineSource::_0;
        solidUnlit->mState.cycle1Combine.alpha[2] = AlphaCombineSource::_0;
        solidUnlit->mState.cycle1Combine.alpha[3] = AlphaCombineSource::PrimitiveAlpha;

        solidUnlit->mState.cycle2Combine = solidUnlit->mState.cycle1Combine;

        solidUnlit->mState.geometryModes.SetFlag((int)GeometryMode::G_LIGHTING, true);
        solidUnlit->mState.geometryModes.SetFlag((int)GeometryMode::G_SHADE, true);

        solidUnlit->mExcludeFromOutut = true;

        settings.mMaterials[SOLID_UNLIT] = solidUnlit;
    }
}

void fillMissingMaterials(TextureCache& cache, const aiScene* fromScene, DisplayListSettings& settings) {
    fillDefaultMaterials(settings);

    for (unsigned i = 0; i < fromScene->mNumMaterials; ++i) {
        aiMaterial* material = fromScene->mMaterials[i];

        std::string materialName = ExtendedMesh::GetMaterialName(material, settings.mForceMaterialName);

        if (settings.mMaterials.find(materialName) != settings.mMaterials.end()) {
            // a material already exists with the given name
            continue;
        }

        aiString diffuseFilename;
        aiString emmisiveFilename;
        aiColor4D diffuseColor;
        aiColor3D emmisiveColor;
        aiTextureMapMode diffuseTextureMapmode;
        aiTextureMapMode emitTextureMapmode;

        bool hasDiffuseTexture = material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseFilename, NULL, NULL, NULL, NULL, &diffuseTextureMapmode) == aiReturn_SUCCESS;
        bool hasEmmissiveTexture = material->GetTexture(aiTextureType_EMISSIVE, 0, &emmisiveFilename, NULL, NULL, NULL, NULL, &emitTextureMapmode) == aiReturn_SUCCESS;

        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        bool hasEmmisiveColor = material->Get(AI_MATKEY_COLOR_EMISSIVE, emmisiveColor) && emmisiveColor != aiColor3D(0, 0, 0);
        
        std::string fallbackMaterial;

        if (hasDiffuseTexture) {
            fallbackMaterial = TEXTURE_LIT;
        } else if (hasEmmissiveTexture) {
            fallbackMaterial = TEXTURE_UNLIT;
        } else if (hasEmmisiveColor) {
            fallbackMaterial = SOLID_LIT;
        } else {
            fallbackMaterial = SOLID_UNLIT;
        }

        auto baseMaterial = settings.mMaterials.find(fallbackMaterial);

        if (baseMaterial == settings.mMaterials.end()) {
            // no base material to start with
            continue;
        }

        std::shared_ptr<Material> result(new Material(*baseMaterial->second));

        result->mName = materialName;
        result->mExcludeFromOutut = false;

        if (hasDiffuseTexture) {
            loadTextureFromAiMaterial(cache, *result, diffuseFilename, settings);
        } else if (hasEmmissiveTexture) {
            loadTextureFromAiMaterial(cache, *result, emmisiveFilename, settings);
        } else if (hasEmmisiveColor) {
            result->mState.usePrimitiveColor = true;
            result->mState.primitiveColor.r = (uint8_t)(emmisiveColor.r * 255.0f);
            result->mState.primitiveColor.g = (uint8_t)(emmisiveColor.g * 255.0f);
            result->mState.primitiveColor.b = (uint8_t)(emmisiveColor.b * 255.0f);
            result->mState.primitiveColor.a = 255;
        } else {
            result->mState.usePrimitiveColor = true;
            result->mState.primitiveColor.r = (uint8_t)(diffuseColor.r * 255.0f);
            result->mState.primitiveColor.g = (uint8_t)(diffuseColor.g * 255.0f);
            result->mState.primitiveColor.b = (uint8_t)(diffuseColor.b * 255.0f);
            result->mState.primitiveColor.a = (uint8_t)(diffuseColor.a * 255.0f);
        }

        if (result->mState.usePrimitiveColor && settings.mTargetCIBuffer && settings.mForcePallete.length()) {
            std::shared_ptr<PalleteDefinition> pallete = gTextureCache.GetPallete(settings.mForcePallete);

            PixelIu8 index = pallete->FindIndex(result->mState.primitiveColor);

            result->mState.primitiveColor.r = index.i;
            result->mState.primitiveColor.g = index.i;
            result->mState.primitiveColor.b = index.i;
        }

        settings.mMaterials[materialName] = result;
    }
}