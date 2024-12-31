#include "TextureCache.h"

#include "../FileUtils.h"

TextureCache gTextureCache;

std::shared_ptr<PaletteDefinition> TextureCache::GetPalette(const std::string& filename) {
    auto check = mPalettes.find(filename);

    if (check != mPalettes.end()) {
        return check->second;
    }

    std::shared_ptr<PaletteDefinition> result(new PaletteDefinition(filename));
    mPalettes[filename] = result;
    return result;
}

std::shared_ptr<TextureDefinition> TextureCache::GetTexture(const std::string& filename, G_IM_FMT format, G_IM_SIZ size, TextureDefinitionEffect effects, const std::string& paletteFilename) {
    std::string normalizedPath = NormalizePath(filename) +
        "#" + nameForImageFormat(format) +
        ":" + nameForImageSize(size) +
        ":" + std::to_string((int)effects) +
        ":" + paletteFilename;

    auto check = mCache.find(normalizedPath);

    if (check != mCache.end()) {
        return check->second;
    }

    std::shared_ptr<PaletteDefinition> palette;

    if (paletteFilename.length()) {
        palette = GetPalette(paletteFilename);
    }

    if (palette) {
        format = G_IM_FMT::G_IM_FMT_CI;

        if (palette->ColorCount() <= 16) {
            size = G_IM_SIZ::G_IM_SIZ_4b;
        } else {
            size = G_IM_SIZ::G_IM_SIZ_8b;
        }
    }

    std::shared_ptr<TextureDefinition> result(new TextureDefinition(filename, format, size, effects, palette));
    mCache[normalizedPath] = result;
    return result;
}