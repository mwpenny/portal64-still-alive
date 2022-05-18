#include "CollisionGenerator.h"

#include "../StringUtils.h"

#include <algorithm>

#define SAME_TOLERANCE  0.00001f

bool bottomRightMost(const aiVector3D& a, const aiVector3D& b) { 
    if (fabs(a.x - b.x) > SAME_TOLERANCE) {
        return a.x < b.x;
    }

    if (fabs(a.y - b.y) > SAME_TOLERANCE) {
        return a.y < b.y;
    }

    return a.z - b.z;
}

const aiVector3D* findMostOppositeEdge(const aiVector3D& fromEdge, const std::vector<aiVector3D>& edges) {
    return std::min_element(edges.begin(), edges.end(), [=](const aiVector3D& a, const aiVector3D& b) {
        return (a * fromEdge) < (b * fromEdge);
    }).base();
}

CollisionQuad::CollisionQuad(aiMesh* mesh, const aiMatrix4x4& transform) {
    if (mesh->mVertices) {
        std::vector<aiVector3D> transformedPoints;

        for (unsigned index = 0; index < mesh->mNumVertices; ++index) {
            transformedPoints.push_back(transform * mesh->mVertices[index]);
        }

        auto cornerPointer = std::min_element(transformedPoints.begin(), transformedPoints.end(), bottomRightMost);
        unsigned cornerIndex = cornerPointer - transformedPoints.begin();

        corner = *cornerPointer;

        std::set<int> adjacentIndices;
        findAdjacentVertices(mesh, cornerIndex, adjacentIndices);
        
        std::vector<aiVector3D> edgesFromCorner;

        for (auto index : adjacentIndices) {
            edgesFromCorner.push_back(transformedPoints[index] - corner);
        }

        auto edgeAPoint = findMostOppositeEdge(edgesFromCorner[0], edgesFromCorner);

        edgeA = *edgeAPoint;
        edgeALength = edgeA.Length();
        edgeA.Normalize();

        auto edgeBPoint = findMostOppositeEdge(edgeA, edgesFromCorner);

        edgeB = *edgeBPoint;
        edgeBLength = edgeB.Length();
        edgeB.Normalize();

        aiMatrix3x3 rotation(transform);

        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            normal += rotation * mesh->mNormals[i];
        }

        normal.Normalize();

        if ((edgeA ^ edgeB) * normal < 0.0f) {
            aiVector3D tmpEdge = edgeA;
            float tmpLength = edgeALength;

            edgeA = edgeB;
            edgeALength = edgeBLength;

            edgeB = tmpEdge;
            edgeBLength = tmpLength;
        }

        corner.x = 0.001f * round(1000.0f * corner.x);
        corner.y = 0.001f * round(1000.0f * corner.y);
        corner.z = 0.001f * round(1000.0f * corner.z);

        edgeA.x = 0.001f * round(1000.0f * edgeA.x);
        edgeA.y = 0.001f * round(1000.0f * edgeA.y);
        edgeA.z = 0.001f * round(1000.0f * edgeA.z);

        edgeALength = 0.001f * round(1000.0f * edgeALength);

        edgeB.x = 0.001f * round(1000.0f * edgeB.x);
        edgeB.y = 0.001f * round(1000.0f * edgeB.y);
        edgeB.z = 0.001f * round(1000.0f * edgeB.z);

        edgeBLength = 0.001f * round(1000.0f * edgeBLength);

        normal.x = 0.001f * round(1000.0f * normal.x);
        normal.y = 0.001f * round(1000.0f * normal.y);
        normal.z = 0.001f * round(1000.0f * normal.z);
    } else {
        corner = aiVector3D();
        edgeA = aiVector3D();
        edgeALength = 0.0f;
        edgeB = aiVector3D();
        edgeBLength = 0.0f;
        normal = aiVector3D();
    }
}

std::unique_ptr<DataChunk> CollisionQuad::Generate() {
    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    result->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(corner)));
    result->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(edgeA)));
    result->AddPrimitive(edgeALength);
    result->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(edgeB)));
    result->AddPrimitive(edgeBLength);

    std::unique_ptr<StructureDataChunk> plane(new StructureDataChunk());
    plane->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(normal)));
    plane->AddPrimitive(-(corner * normal));
    result->Add(std::move(plane));

    result->AddPrimitive(0xF);

    return result;
}

#define FIXED_POINT_PRECISION   8
#define FIXED_POINT_SCALAR      (1 << FIXED_POINT_PRECISION)

void CollisionQuad::ToLocalCoords(const aiVector3D& input, short& outX, short& outY) {
    aiVector3D relative = input - corner;

    outX = (short)(relative * edgeA * FIXED_POINT_SCALAR + 0.5f);
    outY = (short)(relative * edgeB * FIXED_POINT_SCALAR + 0.5f);
}

#define INSIDE_NORMAL_TOLERANCE 0.1f

bool CollisionQuad::IsCoplanar(ExtendedMesh& mesh, float relativeScale) const {
    for (unsigned i = 0; i < mesh.mMesh->mNumVertices; ++i) {
        aiVector3D offset = mesh.mMesh->mVertices[i] * relativeScale - corner;
        
        float z = offset * normal;

        if (fabs(z) >= INSIDE_NORMAL_TOLERANCE) {
            return false;
        }

        float x = offset * edgeA;

        if (x < -INSIDE_NORMAL_TOLERANCE || x > edgeALength + INSIDE_NORMAL_TOLERANCE) {
            return false;
        }

        float y = offset * edgeB;

        if (y < -INSIDE_NORMAL_TOLERANCE || y > edgeBLength + INSIDE_NORMAL_TOLERANCE) {
            return false;
        }
    }

    return true;
}

CollisionGenerator::CollisionGenerator(const DisplayListSettings& settings) : 
    DefinitionGenerator(), 
    mSettings(settings) {}


bool CollisionGenerator::ShouldIncludeNode(aiNode* node) {
    return StartsWith(node->mName.C_Str(), "@collision");
}

void CollisionGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::unique_ptr<StructureDataChunk> collidersChunk(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> colliderTypeChunk(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> collisionObjectChunk(new StructureDataChunk());

    aiMatrix4x4 scale;
    aiMatrix4x4::Scaling(aiVector3D(1, 1, 1) * mSettings.mCollisionScale, scale);
    aiMatrix4x4 rotation(mSettings.mRotateModel.GetMatrix());

    aiMatrix4x4 globalTransform = rotation * scale;

    int meshCount = 0;

    std::string quadCollidersName = fileDefinition.GetUniqueName("quad_colliders");
    std::string colliderTypesName = fileDefinition.GetUniqueName("collider_types");
    std::string collisionObjectsName = fileDefinition.GetUniqueName("collision_objects");

    for (auto node = mIncludedNodes.begin(); node != mIncludedNodes.end(); ++node) {
        for (unsigned i = 0; i < (*node)->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[(*node)->mMeshes[i]];

            CollisionQuad collider(mesh, globalTransform * (*node)->mTransformation);
            collidersChunk->Add(std::move(collider.Generate()));

            std::unique_ptr<StructureDataChunk> colliderType(new StructureDataChunk());
            colliderType->AddPrimitive<const char*>("CollisionShapeTypeQuad");
            colliderType->AddPrimitive(std::string("&" + quadCollidersName + "[" + std::to_string(meshCount) + "]"));
            colliderType->AddPrimitive(0.0f);
            colliderType->AddPrimitive(1.0f);
            colliderType->AddPrimitive<const char*>("NULL");
            colliderTypeChunk->Add(std::move(colliderType));

            std::unique_ptr<StructureDataChunk> collisionObject(new StructureDataChunk());
            collisionObject->AddPrimitive(std::string("&" + colliderTypesName + "[" + std::to_string(meshCount) + "]"));
            collisionObject->AddPrimitive<const char*>("NULL");
            collisionObjectChunk->Add(std::move(collisionObject));

            mOutput.quads.push_back(collider);

            ++meshCount;
        }
    }

    std::unique_ptr<FileDefinition> collisionFileDef(new DataFileDefinition(
        "struct CollisionQuad", 
        quadCollidersName, 
        true, 
        "_geo", 
        std::move(collidersChunk)
    ));

    collisionFileDef->AddTypeHeader("\"physics/collision_quad.h\"");
    collisionFileDef->AddTypeHeader("\"physics/collision.h\"");
    collisionFileDef->AddTypeHeader("\"physics/collision_object.h\"");
    
    fileDefinition.AddDefinition(std::move(collisionFileDef));

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition(
        "struct ColliderTypeData", 
        colliderTypesName, 
        true, 
        "_geo", 
        std::move(colliderTypeChunk)
    )));

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition(
        "struct CollisionObject", 
        collisionObjectsName, 
        true, 
        "_geo", 
        std::move(collisionObjectChunk)
    )));

    fileDefinition.AddMacro(fileDefinition.GetMacroName("QUAD_COLLIDERS_COUNT"), std::to_string(meshCount));

    mOutput.quadsName = collisionObjectsName;
}

const CollisionGeneratorOutput& CollisionGenerator::GetOutput() const {
    return mOutput;
}