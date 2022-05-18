#ifndef __TEXTURE_CACHE_H__
#define __TEXTURE_CACHE_H__

#include "TextureDefinition.h"
#include <memory>
#include <map>
#include <string>

class TextureCache {
public:
    std::shared_ptr<TextureDefinition> GetTexture(const std::string& filename, G_IM_FMT format, G_IM_SIZ size, TextureDefinitionEffect effects);
private:
    std::map<std::string, std::shared_ptr<TextureDefinition>> mCache;
};

#endif