#include "CFileDefinition.h"
#include <stdio.h>
#include <iostream>
#include "StringUtils.h"
#include "FileUtils.h"

#include <fstream>

VertexBufferDefinition::VertexBufferDefinition(std::shared_ptr<ExtendedMesh> targetMesh, std::string name, VertexType vertexType, int textureWidth, int textureHeight):
    mTargetMesh(targetMesh),
    mName(name),
    mVertexType(vertexType),
    mTextureWidth(textureWidth),
    mTextureHeight(textureHeight) {

}

ErrorResult convertToShort(float value, short& output) {
    int result = (int)floor(value + 0.5);

    if (result < (int)std::numeric_limits<short>::min() || result > (int)std::numeric_limits<short>::max()) {
        return ErrorResult("The value " + std::to_string(result) + " is too big to fit into a short");
    }

    output = (short)result;

    return ErrorResult();
}

int convertNormalizedRange(float value) {
    int result = (int)(value * 128.0f);

    if (result < std::numeric_limits<char>::min()) {
        return std::numeric_limits<char>::min();
    } else if (result > std::numeric_limits<char>::max()) {
        return std::numeric_limits<char>::max();
    } else {
        return (char)result;
    }
}

unsigned convertByteRange(float value) {
    int result = (int)(value * 256.0f);

    if (result < std::numeric_limits<unsigned char>::min()) {
        return std::numeric_limits<unsigned char>::min();
    } else if (result > std::numeric_limits<unsigned char>::max()) {
        return std::numeric_limits<unsigned char>::max();
    } else {
        return (unsigned char)result;
    }
}

 ErrorResult VertexBufferDefinition::Generate(float fixedPointScale, float modelScale, aiQuaternion rotate, std::unique_ptr<FileDefinition>& output, const std::string& fileSuffix) {
    std::unique_ptr<StructureDataChunk> dataChunk(new StructureDataChunk());

    // aiQuaternion rotateInverse = rotate;
    // rotateInverse.Conjugate();
    
    for (unsigned int i = 0; i < mTargetMesh->mMesh->mNumVertices; ++i) {
        std::unique_ptr<StructureDataChunk> vertexWrapper(new StructureDataChunk());

        std::unique_ptr<StructureDataChunk> vertex(new StructureDataChunk());

        aiVector3D pos = mTargetMesh->mMesh->mVertices[i];

        if (mTargetMesh->mPointInverseTransform.size()) {
            pos = mTargetMesh->mPointInverseTransform[i] * pos;
        } else {
            pos = rotate.Rotate(pos) * modelScale;
        }

        pos = pos * fixedPointScale;

        short converted;

        std::unique_ptr<StructureDataChunk> posVertex(new StructureDataChunk());

        ErrorResult code = convertToShort(pos.x, converted);
        if (code.HasError()) return ErrorResult(code.GetMessage() + " for x coordinate");
        posVertex->AddPrimitive(converted);
        
        code = convertToShort(pos.y, converted);
        if (code.HasError()) return ErrorResult(code.GetMessage() + " for y coordinate");
        posVertex->AddPrimitive(converted);

        code = convertToShort(pos.z, converted);
        if (code.HasError()) return ErrorResult(code.GetMessage() + " for z coordinate");
        posVertex->AddPrimitive(converted);

        vertex->Add(std::move(posVertex));
        vertex->AddPrimitive(0);


        std::unique_ptr<StructureDataChunk> texCoords(new StructureDataChunk());

        if (mTargetMesh->mMesh->mTextureCoords[0] == nullptr) {
            texCoords->AddPrimitive(0);
            texCoords->AddPrimitive(0);
        } else {
            aiVector3D uv = mTargetMesh->mMesh->mTextureCoords[0][i];

            code = convertToShort(uv.x * mTextureWidth * (1 << 5), converted);
            if (code.HasError()) return ErrorResult(code.GetMessage() + " for texture u coordinate");
            texCoords->AddPrimitive(converted);

            code = convertToShort((1.0f - uv.y) * mTextureHeight * (1 << 5), converted);
            if (code.HasError()) return ErrorResult(code.GetMessage() + " for texture y coordinate");
            texCoords->AddPrimitive(converted);
        }

        vertex->Add(std::move(texCoords));

        std::unique_ptr<StructureDataChunk> vertexNormal(new StructureDataChunk());

        switch (mVertexType) {
        case VertexType::PosUVNormal:
        case VertexType::POSUVTangent:
        case VertexType::POSUVMinusTangent:
        case VertexType::POSUVCotangent:
        case VertexType::POSUVMinusCotangent:
        {
            aiVector3D normal;

            switch (mVertexType) {
            case VertexType::PosUVNormal:
                if (mTargetMesh->mMesh->HasNormals()) {
                    normal = mTargetMesh->mMesh->mNormals[i];
                }
                break;
            case VertexType::POSUVTangent:
                if (mTargetMesh->mMesh->mTangents) {
                    normal = mTargetMesh->mMesh->mTangents[i];
                }
                break;
            case VertexType::POSUVMinusTangent:
                if (mTargetMesh->mMesh->mTangents) {
                    normal = -mTargetMesh->mMesh->mTangents[i];
                }
                break;
            case VertexType::POSUVCotangent:
                if (mTargetMesh->mMesh->mBitangents) {
                    normal = mTargetMesh->mMesh->mBitangents[i];
                }
                break;
            case VertexType::POSUVMinusCotangent:
                if (mTargetMesh->mMesh->mBitangents) {
                    normal = -mTargetMesh->mMesh->mBitangents[i];
                }
                break;
                default:
                break;
            }

            if (mTargetMesh->mPointInverseTransform.size()) {
                normal = mTargetMesh->mNormalInverseTransform[i] * normal;
                normal.NormalizeSafe();
            } else {
                normal = rotate.Rotate(normal);
            }

            float a = 1.0f;

            if (mTargetMesh->mMesh->mColors[1] != nullptr) {
                a = mTargetMesh->mMesh->mColors[1][i].r;
            }

            vertexNormal->AddPrimitive(convertNormalizedRange(normal.x));
            vertexNormal->AddPrimitive(convertNormalizedRange(normal.y));
            vertexNormal->AddPrimitive(convertNormalizedRange(normal.z));
            vertexNormal->AddPrimitive(convertByteRange(a));
            break;
        }
        case VertexType::PosUVColor:
            if (mTargetMesh->mMesh->mColors[0] != nullptr) {
                aiColor4D color = mTargetMesh->mMesh->mColors[0][i];
                
                if (mTargetMesh->mMesh->mColors[1] != nullptr) {
                    color.a = mTargetMesh->mMesh->mColors[1][i].r;
                }

                vertexNormal->AddPrimitive(convertByteRange(color.r));
                vertexNormal->AddPrimitive(convertByteRange(color.g));
                vertexNormal->AddPrimitive(convertByteRange(color.b));
                vertexNormal->AddPrimitive(convertByteRange(color.a));
            } else {
                vertexNormal->AddPrimitive(0);
                vertexNormal->AddPrimitive(0);
                vertexNormal->AddPrimitive(0);
                vertexNormal->AddPrimitive(255);
            }
            break;
        }

        vertex->Add(std::move(vertexNormal));

        vertexWrapper->Add(std::move(vertex));
        dataChunk->Add(std::move(vertexWrapper));
    }

    output = std::unique_ptr<FileDefinition>(new DataFileDefinition("Vtx", mName, true, fileSuffix, std::move(dataChunk)));

    return ErrorResult();
}

CFileDefinition::CFileDefinition(std::string prefix, float fixedPointScale, float modelScale, aiQuaternion modelRotate): 
    mPrefix(prefix),
    mFixedPointScale(fixedPointScale),
    mModelScale(modelScale),
    mModelRotate(modelRotate) {

}

void CFileDefinition::AddDefinition(std::unique_ptr<FileDefinition> definition) {
    if (definition->ForResource()) {
        mResourceNames[definition->ForResource()] = definition->GetName();
    }

    mDefinitions.push_back(std::move(definition));
}

std::string CFileDefinition::AddDataDefinition(const std::string& nameHint, const std::string& dataType, bool isArray, const std::string& location, std::unique_ptr<DataChunk> data) {
    std::string name = GetUniqueName(nameHint);
    std::unique_ptr<FileDefinition> def(new DataFileDefinition(dataType, name, isArray, location, std::move(data)));
    AddDefinition(std::move(def));
    return name;
}

void CFileDefinition::AddMacro(const std::string& name, const std::string& value) {
    mMacros.push_back(name + " " + value);
}

void CFileDefinition::AddHeader(const std::string& name) {
    mHeaders.insert(name);
}

std::string CFileDefinition::GetVertexBuffer(std::shared_ptr<ExtendedMesh> mesh, VertexType vertexType, int textureWidth, int textureHeight, const std::string& modelSuffix) {
    for (auto existing = mVertexBuffers.begin(); existing != mVertexBuffers.end(); ++existing) {
        if (existing->second.mTargetMesh == mesh && existing->second.mVertexType == vertexType) {
            return existing->first;
        }
    }

    std::string requestedName;

    if (mesh->mMesh->mName.length) {
        requestedName = mesh->mMesh->mName.C_Str();
    } else {
        requestedName = "_mesh";
    }

    switch (vertexType) {
        case VertexType::PosUVColor:
            requestedName += "_color";
            break;
        case VertexType::PosUVNormal:
            requestedName += "_normal";
            break;
        case VertexType::POSUVTangent:
            requestedName += "_tangent";
            break;
        case VertexType::POSUVMinusTangent:
            requestedName += "_ntangent";
            break;
        case VertexType::POSUVCotangent:
            requestedName += "_bitangent";
            break;
        case VertexType::POSUVMinusCotangent:
            requestedName += "_nbitangent";
            break;

    }



    std::string name = GetUniqueName(requestedName);


    mVertexBuffers.insert(std::pair<std::string, VertexBufferDefinition>(name, VertexBufferDefinition(
        mesh, 
        name, 
        vertexType,
        textureWidth,
        textureHeight
    )));

    std::unique_ptr<FileDefinition> vtxDef;

    ErrorResult result = mVertexBuffers.find(name)->second.Generate(mFixedPointScale, mModelScale, mModelRotate, vtxDef, modelSuffix);

    if (result.HasError()) {
        std::cerr << "Error generating vertex buffer " << name << " error: " << result.GetMessage() << std::endl;
    } else {
        AddDefinition(std::move(vtxDef));
    }

    return name;
}

std::string CFileDefinition::GetCullingBuffer(const std::string& name, const aiVector3D& min, const aiVector3D& max, const std::string& modelSuffix) {
    aiMesh* mesh = new aiMesh();

    mesh->mName = name;
    mesh->mNumVertices = 8;
    mesh->mVertices = new aiVector3D[8];
    mesh->mVertices[0] = aiVector3D(min.x, min.y, min.z);
    mesh->mVertices[1] = aiVector3D(min.x, min.y, max.z);
    mesh->mVertices[2] = aiVector3D(min.x, max.y, min.z);
    mesh->mVertices[3] = aiVector3D(min.x, max.y, max.z);
    mesh->mVertices[4] = aiVector3D(max.x, min.y, min.z);
    mesh->mVertices[5] = aiVector3D(max.x, min.y, max.z);
    mesh->mVertices[6] = aiVector3D(max.x, max.y, min.z);
    mesh->mVertices[7] = aiVector3D(max.x, max.y, max.z);

    BoneHierarchy boneHierarchy;
    return GetVertexBuffer(std::shared_ptr<ExtendedMesh>(new ExtendedMesh(mesh, boneHierarchy)), VertexType::PosUVNormal, 0, 0, modelSuffix);
}


std::string CFileDefinition::GetUniqueName(std::string requestedName) {
    std::string result = mPrefix + "_" + requestedName;
    makeCCompatible(result);

    int index = 1;
    
    while (mUsedNames.find(result) != mUsedNames.end()) {
        char strBuffer[8];
        snprintf(strBuffer, 8, "_%d", index);
        result = mPrefix + "_" + requestedName + strBuffer;
        makeCCompatible(result);
        ++index;
    }

    mUsedNames.insert(result);

    return result;
}


std::string CFileDefinition::GetMacroName(std::string requestedName) {
    std::string result = GetUniqueName(requestedName);

    std::transform(result.begin(), result.end(), result.begin(), ::toupper);

    return result;
}


void CFileDefinition::GenerateAll(const std::string& headerFileLocation) {
    std::set<std::string> keys;

    for (auto fileDef = mDefinitions.begin(); fileDef != mDefinitions.end(); ++fileDef) {
        keys.insert((*fileDef)->GetLocation());
    }

    std::string fileNoExtension = replaceExtension(headerFileLocation, "");
    std::string fileNoPath = getBaseName(fileNoExtension);

    for (auto key : keys) {
        std::ofstream outputFile;
        outputFile.open(fileNoExtension + key + ".c", std::ios_base::out | std::ios_base::trunc);
        Generate(outputFile, key, fileNoPath + ".h");
        outputFile.close();
    }

    std::ofstream outputHeader;
    outputHeader.open(fileNoExtension + ".h", std::ios_base::out | std::ios_base::trunc);
    GenerateHeader(outputHeader, fileNoPath);
    outputHeader.close();
}

void CFileDefinition::Generate(std::ostream& output, const std::string& location, const std::string& headerFileName) {
    output << "#include \"" << headerFileName << "\"" << std::endl;

    for (auto it = mDefinitions.begin(); it != mDefinitions.end(); ++it) {

        if ((*it)->GetLocation() == location) {
            (*it)->Generate(output);

            output << ";\n\n";
        }
    }
}

void CFileDefinition::GenerateHeader(std::ostream& output, const std::string& headerFileName) {
    std::string infdef = std::string("__SKOUT_") + headerFileName + "_H__";

    makeCCompatible(infdef);
    std::transform(infdef.begin(), infdef.end(), infdef.begin(), ::toupper);

    output << "#ifndef " << infdef << std::endl;
    output << "#define " << infdef << std::endl;
    output << std::endl;

    std::set<std::string> includes = mHeaders;

    for (auto it = mDefinitions.begin(); it != mDefinitions.end(); ++it) {
        auto headers = (*it)->GetTypeHeaders();

        for (auto header : headers) {
            includes.insert(header);
        }
    }

    std::vector<std::string> includesSorted(includes.size());
    std::copy(includes.begin(), includes.end(), includesSorted.begin());

    // std::sort(includes.begin(), includes.end());

    for (auto include : includesSorted) {
        output << "#include " << include << std::endl;
    }

    output << std::endl;

    if (mMacros.size()) {
        for (auto macro : mMacros) {
            output << "#define " << macro << std::endl;
        }
        output << std::endl;
    }

    for (auto it = mDefinitions.begin(); it != mDefinitions.end(); ++it) {
            (*it)->GenerateDeclaration(output);
            output << ";\n";
    };

    output << std::endl;
    output << "#endif" << std::endl;
}

bool CFileDefinition::HasDefinitions(const std::string& location) {
    for (auto it = mDefinitions.begin(); it != mDefinitions.end(); ++it) {
        if ((*it)->GetLocation() == location) {
            return true;
        }
    }

    return false;
}


bool CFileDefinition::GetResourceName(const void* resource, std::string& result) const {
    auto it = mResourceNames.find(resource);

    if (it != mResourceNames.end()) {
        result = it->second;
        return true;
    }

    return false;
}

std::shared_ptr<ExtendedMesh> CFileDefinition::GetExtendedMesh(aiMesh* mesh) {
    auto it = mMeshes.find(mesh);

    if (it != mMeshes.end()) {
        return it->second;
    }

    std::shared_ptr<ExtendedMesh> result(new ExtendedMesh(mesh, mBoneHierarchy));

    mMeshes[mesh] = result;

    return result;
}

BoneHierarchy& CFileDefinition::GetBoneHierarchy() {
    return mBoneHierarchy;
}