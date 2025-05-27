#ifndef __TEXTURE_CACHE_H__
#define __TEXTURE_CACHE_H__

#include "TextureDefinition.h"
#include <memory>
#include <map>
#include <string>

class TextureCache {
public:
    std::shared_ptr<PaletteDefinition> GetPalette(const std::string& filename);
    std::shared_ptr<TextureDefinition> GetTexture(const std::string& filename, G_IM_FMT format, G_IM_SIZ size, TextureDefinitionEffect effects, const std::string& paletteFilename);
private:
    std::map<std::string, std::shared_ptr<PaletteDefinition>> mPalettes;
    std::map<std::string, std::shared_ptr<TextureDefinition>> mCache;
};

extern TextureCache gTextureCache;

#endif