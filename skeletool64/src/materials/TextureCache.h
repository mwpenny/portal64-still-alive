#ifndef __TEXTURE_CACHE_H__
#define __TEXTURE_CACHE_H__

#include "TextureDefinition.h"
#include <memory>
#include <map>
#include <string>

class TextureCache {
public:
    std::shared_ptr<PalleteDefinition> GetPallete(const std::string& filename);
    std::shared_ptr<TextureDefinition> GetTexture(const std::string& filename, G_IM_FMT format, G_IM_SIZ size, TextureDefinitionEffect effects, const std::string& palleteFilename);
private:
    std::map<std::string, std::shared_ptr<PalleteDefinition>> mPalletes;
    std::map<std::string, std::shared_ptr<TextureDefinition>> mCache;
};

extern TextureCache gTextureCache;

#endif