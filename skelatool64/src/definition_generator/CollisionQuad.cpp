#include "CollisionQuad.h"

#include "../MathUtl.h"

#define SAME_TOLERANCE  0.00001f

bool bottomRightMost(const aiVector3D& a, const aiVector3D& b) { 
    if (fabs(a.x - b.x) > SAME_TOLERANCE) {
        return a.x < b.x;
    }

    if (fabs(a.y - b.y) > SAME_TOLERANCE) {
        return a.y < b.y;
    }

    return a.z < b.z;
}

const aiVector3D* findMostOppositeEdge(const aiVector3D& fromEdge, const std::vector<aiVector3D>& edges) {
    return std::min_element(edges.begin(), edges.end(), [=](const aiVector3D& a, const aiVector3D& b) {
        return (a * fromEdge) < (b * fromEdge);
    }).base();
}

CollisionQuad::CollisionQuad(aiMesh* mesh, const aiMatrix4x4& transform): thickness(0.0f) {
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

std::unique_ptr<DataChunk> CollisionQuad::Generate() const {
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

    result->AddPrimitive(thickness);

    return result;
}

#define INSIDE_NORMAL_TOLERANCE 0.1f

bool CollisionQuad::IsCoplanar(ExtendedMesh& mesh, float relativeScale) const {
    for (unsigned i = 0; i < mesh.mMesh->mNumVertices; ++i) {
        aiVector3D offset = mesh.mMesh->mVertices[i] * relativeScale - corner;
        
        float z = offset * normal;

        if (fabs(z) >= INSIDE_NORMAL_TOLERANCE) {
            return false;
        }
    }

    return true;
}

bool CollisionQuad::IsCoplanar(const aiVector3D& input) const {
    aiVector3D offset = input - corner;
    
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

    return true;
}

aiAABB CollisionQuad::BoundingBox() const {
    aiAABB result;

    result.mMin = corner;
    result.mMax = corner;

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                aiVector3D point = corner;

                if (x) {
                    point = point + edgeA * edgeALength;
                }

                if (y) {
                    point = point + edgeB * edgeBLength;
                }

                if (z) {
                    point = point - normal * thickness;
                }
                
                result.mMin = min(result.mMin, point);
                result.mMax = max(result.mMax, point);
            }
        }
    }

    return result;
}