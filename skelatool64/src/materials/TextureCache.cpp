#include "TextureCache.h"

#include "../FileUtils.h"

std::shared_ptr<TextureDefinition> TextureCache::GetTexture(const std::string& filename, G_IM_FMT format, G_IM_SIZ size, TextureDefinitionEffect effects) {
    std::string normalizedPath = NormalizePath(filename) +
        "#" + nameForImageFormat(format) +
        ":" + nameForImageSize(size) +
        ":" + std::to_string((int)effects);

    auto check = mCache.find(normalizedPath);

    if (check != mCache.end()) {
        return check->second;
    }

    std::shared_ptr<TextureDefinition> result(new TextureDefinition(filename, format, size, effects));
    mCache[normalizedPath] = result;
    return result;
}