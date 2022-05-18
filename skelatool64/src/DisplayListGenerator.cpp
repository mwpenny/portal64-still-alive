#include <set>
#include <vector>
#include <algorithm>
#include <map>

#include "./DisplayListGenerator.h"

bool doesFaceFit(std::set<int>& indices, aiFace* face, unsigned int maxVertices) {
    unsigned int misses = 0;

    for (unsigned int i = 0; i < face->mNumIndices; ++i) {
        if (indices.find(face->mIndices[i]) == indices.end()) {
            ++misses;
        }
    }

    return indices.size() + misses <= maxVertices;
}

void flushVertices(RenderChunk& chunk, std::set<int>& currentVertices, std::vector<aiFace*>& currentFaces, RCPState& state, std::string vertexBuffer, DisplayList& output, bool hasTri2) {
    std::vector<int> verticesAsVector(currentVertices.begin(), currentVertices.end());

    std::sort(verticesAsVector.begin(), verticesAsVector.end(), 
        [=](int a, int b) -> bool {
        Bone* boneA = chunk.mMesh->mVertexBones[a];
        Bone* boneB = chunk.mMesh->mVertexBones[b];

        if (boneA != boneB && (boneA == chunk.mBonePair.first || boneB == chunk.mBonePair.second)) {
            return true;
        }

        return a < b;
    });

    VertexData vertexData[MAX_VERTEX_CACHE_SIZE];

    for (unsigned int vertexIndex = 0; vertexIndex < verticesAsVector.size() && vertexIndex < MAX_VERTEX_CACHE_SIZE; ++vertexIndex) {
        vertexData[vertexIndex] = VertexData(vertexBuffer, verticesAsVector[vertexIndex], -1);
    }
    
    unsigned int cacheLocation[MAX_VERTEX_CACHE_SIZE];

    state.AssignSlots(vertexData, cacheLocation, verticesAsVector.size());
    int lastVertexIndex = -1;
    int lastCacheLocation = MAX_VERTEX_CACHE_SIZE;
    int vertexCount = 0;
    Bone* lastBone = nullptr;
    std::map<int, int> vertexMapping;

    for (unsigned int index = 0; index <= verticesAsVector.size(); ++index) {
        int vertexIndex;
        int cacheIndex;
        Bone* bone = nullptr;
        
        if (index < verticesAsVector.size()) {
            vertexIndex = verticesAsVector[index];
            cacheIndex = cacheLocation[index];
            vertexMapping[vertexIndex] = cacheIndex;
            bone = chunk.mMesh->mVertexBones[vertexIndex];
        }

        if (index == verticesAsVector.size() || 
            (index != 0 && (
                vertexIndex != lastVertexIndex + 1 || 
                cacheIndex != lastCacheLocation + 1 || bone != lastBone
            ))) {
            state.TraverseToBone(lastBone, output);
            output.AddCommand(std::unique_ptr<DisplayListCommand>(new VTXCommand(
                vertexCount, 
                lastCacheLocation + 1 - vertexCount, 
                vertexBuffer, 
                lastVertexIndex + 1 - vertexCount
            )));

            vertexCount = 1;
        } else {
            ++vertexCount;
        }

        lastVertexIndex = vertexIndex;
        lastCacheLocation = cacheIndex;
        lastBone = bone;
    }

    for (unsigned int faceIndex = 0; faceIndex < currentFaces.size(); ++faceIndex) {
        if (hasTri2 && faceIndex + 1 < currentFaces.size()) {
            aiFace* currFace = currentFaces[faceIndex + 0];
            aiFace* nextFace = currentFaces[faceIndex + 1];

            output.AddCommand(std::unique_ptr<DisplayListCommand>(new TRI2Command(
                vertexMapping.at(currFace->mIndices[0]), vertexMapping.at(currFace->mIndices[1]), vertexMapping.at(currFace->mIndices[2]),
                vertexMapping.at(nextFace->mIndices[0]), vertexMapping.at(nextFace->mIndices[1]), vertexMapping.at(nextFace->mIndices[2])
            )));

            ++faceIndex;
        } else {
            aiFace* currFace = currentFaces[faceIndex + 0];

            output.AddCommand(std::unique_ptr<DisplayListCommand>(new TRI1Command(
                vertexMapping.at(currFace->mIndices[0]), vertexMapping.at(currFace->mIndices[1]), vertexMapping.at(currFace->mIndices[2])
            )));
        }
    }
}

void generateCulling(DisplayList& output, std::string vertexBuffer, bool renableLighting) {
    output.AddCommand(std::unique_ptr<DisplayListCommand>(new ChangeGeometryMode(GeometryMode::G_LIGHTING, GeometryMode::None)));
    output.AddCommand(std::unique_ptr<DisplayListCommand>(new VTXCommand(8, 0, vertexBuffer, 0)));
    output.AddCommand(std::unique_ptr<DisplayListCommand>(new CullDisplayList(8)));
    if (renableLighting) {
        output.AddCommand(std::unique_ptr<DisplayListCommand>(new ChangeGeometryMode(GeometryMode::None, GeometryMode::G_LIGHTING)));
    }
}

void generateGeometry(RenderChunk& chunk, RCPState& state, std::string vertexBuffer, DisplayList& output, bool hasTri2) {
    std::set<int> currentVertices;
    std::vector<aiFace*> currentFaces;

    const std::vector<aiFace*>& faces = chunk.GetFaces();

    for (unsigned int faceIndex = 0; faceIndex <= faces.size(); ++faceIndex) {
        if (faceIndex == faces.size() || !doesFaceFit(currentVertices, faces[faceIndex], state.GetMaxVertices())) {
            flushVertices(chunk, currentVertices, currentFaces, state, vertexBuffer, output, hasTri2);

            currentVertices.clear();
            currentFaces.clear();

            if (faceIndex == faces.size()) {
                break;
            }
        }

        for (unsigned int vertexIndex = 0; vertexIndex < faces[faceIndex]->mNumIndices; ++vertexIndex) {
            currentVertices.insert(faces[faceIndex]->mIndices[vertexIndex]);
        }

        currentFaces.push_back(faces[faceIndex]);
    }
}