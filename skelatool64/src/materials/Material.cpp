
#include "Material.h"

#include "../StringUtils.h"
#include "../CFileDefinition.h"

Material::Material(const std::string& name): mName(name) {}

void Material::Write(CFileDefinition& fileDef, const MaterialState& from, StructureDataChunk& output) {
    generateMaterial(fileDef, from, mState, output);
}

int Material::TextureWidth(Material* material) {
    if (!material) {
        return 0;
    }

    for (int i = 0; i < MAX_TILE_COUNT; ++i) {
        if (material->mState.tiles[i].isOn && material->mState.tiles[i].texture) {
            return material->mState.tiles[i].texture->Width();
        }
    }

    return 0;
}

int Material::TextureHeight(Material* material) {
    if (!material) {
        return 0;
    }

    for (int i = 0; i < MAX_TILE_COUNT; ++i) {
        if (material->mState.tiles[i].isOn && material->mState.tiles[i].texture) {
            return material->mState.tiles[i].texture->Height();
        }
    }

    return 0;
}

VertexType Material::GetVertexType(Material* material) {
    if (!material) {
        return VertexType::PosUVNormal;
    }

    if (material->mState.geometryModes.knownFlags & material->mState.geometryModes.flags & (int)GeometryMode::G_LIGHTING) {
        return VertexType::PosUVNormal;
    }

    return VertexType::PosUVColor;
}