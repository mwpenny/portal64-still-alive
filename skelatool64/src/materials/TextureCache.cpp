#include "TextureCache.h"

#include "../FileUtils.h"

TextureCache gTextureCache;

std::shared_ptr<PalleteDefinition> TextureCache::GetPallete(const std::string& filename) {
    auto check = mPalletes.find(filename);

    if (check != mPalletes.end()) {
        return check->second;
    }

    std::shared_ptr<PalleteDefinition> result(new PalleteDefinition(filename));
    mPalletes[filename] = result;
    return result;
}

std::shared_ptr<TextureDefinition> TextureCache::GetTexture(const std::string& filename, G_IM_FMT format, G_IM_SIZ size, TextureDefinitionEffect effects, const std::string& palleteFilename) {
    std::string normalizedPath = NormalizePath(filename) +
        "#" + nameForImageFormat(format) +
        ":" + nameForImageSize(size) +
        ":" + std::to_string((int)effects) +
        ":" + palleteFilename;

    auto check = mCache.find(normalizedPath);

    if (check != mCache.end()) {
        return check->second;
    }

    std::shared_ptr<PalleteDefinition> pallete;

    if (palleteFilename.length()) {
        pallete = GetPallete(palleteFilename);
    }

    if (pallete) {
        format = G_IM_FMT::G_IM_FMT_CI;

        if (pallete->ColorCount() <= 16) {
            size = G_IM_SIZ::G_IM_SIZ_4b;
        } else {
            size = G_IM_SIZ::G_IM_SIZ_8b;
        }
    }

    std::shared_ptr<TextureDefinition> result(new TextureDefinition(filename, format, size, effects, pallete));
    mCache[normalizedPath] = result;
    return result;
}