
#include "Material.h"

#include "../StringUtils.h"
#include "../CFileDefinition.h"

Material::Material(const std::string& name): mName(name), mNormalSource(NormalSource::Normal), mExcludeFromOutut(false), mSortOrder(0) {}

void Material::Write(CFileDefinition& fileDef, const MaterialState& from, StructureDataChunk& output, bool targetCIBuffer) {
    generateMaterial(fileDef, from, mState, output, targetCIBuffer);
}

int Material::TextureWidth(Material* material) {
    if (!material) {
        return 0;
    }

    int tileIndex = material->mState.textureState.tile;
    TileState& tile = material->mState.tiles[tileIndex];

    return (tile.isOn && tile.texture) ? tile.texture->Width() : 0;
}

int Material::TextureHeight(Material* material) {
    if (!material) {
        return 0;
    }

    int tileIndex = material->mState.textureState.tile;
    TileState& tile = material->mState.tiles[tileIndex];

    return (tile.isOn && tile.texture) ? tile.texture->Height() : 0;
}

VertexType convertNormalSourceToVertexType(NormalSource normalSource) {
    switch (normalSource) {
        case NormalSource::Normal:
            return VertexType::PosUVNormal;

        case NormalSource::Tangent:
            return VertexType::POSUVTangent;
        case NormalSource::MinusTangent:
            return VertexType::POSUVMinusTangent;
        case NormalSource::Bitangent:
            return VertexType::POSUVMinusCotangent;
        case NormalSource::MinusCotangent:
            return VertexType::POSUVMinusCotangent;
    }

    return VertexType::PosUVNormal;
}

VertexType Material::GetVertexType(Material* material) {
    if (!material) {
        return VertexType::PosUVNormal;
    }

    if (material->mState.geometryModes.knownFlags & 
        material->mState.geometryModes.flags & 
        ((int)GeometryMode::G_LIGHTING | (int)GeometryMode::G_TEXTURE_GEN)) {
        return convertNormalSourceToVertexType(material->mNormalSource);
    }

    return VertexType::PosUVColor;
}