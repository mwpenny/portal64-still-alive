#ifndef _C_FILE_DEFINITION_H
#define _C_FILE_DEFINITION_H

#include <assimp/mesh.h>
#include <map>
#include <string>
#include <set>
#include <ostream>
#include <memory>

#include "./ErrorResult.h"
#include "./ExtendedMesh.h"
#include "./definitions/FileDefinition.h"
#include "./materials/TextureDefinition.h"

class VertexBufferDefinition {
public:
    VertexBufferDefinition(std::shared_ptr<ExtendedMesh> targetMesh, std::string name, VertexType vertexType, int textureWidth, int textureHeight);

    std::shared_ptr<ExtendedMesh> mTargetMesh;
    std::string mName;
    VertexType mVertexType;
    int mTextureWidth;
    int mTextureHeight;

    ErrorResult Generate(float fixedPointScale, float modelScale, aiQuaternion rotate, std::unique_ptr<FileDefinition>& output, const std::string& fileSuffix, const PixelRGBAu8& defaultVertexColor);
private:
};

class CFileDefinition {
public:
    CFileDefinition(std::string prefix, float fixedPointScale, float modelScale, aiQuaternion modelRotate);

    void AddDefinition(std::unique_ptr<FileDefinition> definition);
    std::string AddDataDefinition(const std::string& nameHint, const std::string& dataType, bool isArray, const std::string& location, std::unique_ptr<DataChunk> data);
    void AddMacro(const std::string& name, const std::string& value);

    void AddHeader(const std::string& name);

    std::string GetVertexBuffer(std::shared_ptr<ExtendedMesh> mesh, VertexType vertexType, int textureWidth, int textureHeight, const std::string& modelSuffix, const PixelRGBAu8& defaultVertexColor);
    std::string GetCullingBuffer(const std::string& name, const aiVector3D& min, const aiVector3D& max, const std::string& modelSuffix);

    std::string GetUniqueName(std::string requestedName);

    std::string GetMacroName(std::string requestedName);

    std::set<std::string> GetDefinitionTypes();

    void GenerateAll(const std::string& headerFileLocation);

    void Generate(std::ostream& output, const std::string& location, const std::string& headerFileName);
    void GenerateHeader(std::ostream& output, const std::string& headerFileName);

    bool HasDefinitions(const std::string& location);

    bool GetResourceName(const void* resource, std::string& result) const;

    std::shared_ptr<ExtendedMesh> GetExtendedMesh(aiMesh* mesh);

    BoneHierarchy& GetBoneHierarchy();
private:
    std::string mPrefix;
    float mFixedPointScale;
    float mModelScale;
    aiQuaternion mModelRotate;
    std::set<std::string> mHeaders;
    std::set<std::string> mUsedNames;
    std::map<std::string, VertexBufferDefinition> mVertexBuffers;
    std::vector<std::unique_ptr<FileDefinition>> mDefinitions;
    std::vector<std::string> mMacros;
    std::map<const void*, std::string> mResourceNames;
    std::map<aiMesh*, std::shared_ptr<ExtendedMesh>> mMeshes;
    BoneHierarchy mBoneHierarchy;
};

#endif