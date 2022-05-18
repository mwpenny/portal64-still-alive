#include "LevelGenerator.h"

#include <set>
#include <string>
#include "../math/MES.h"

std::set<std::string> gPortalableSurfaces = {
    "concrete_modular_wall001d",
    "concrete_modular_ceiling001a",
    "concrete_modular_floor001a",
};

LevelGenerator::LevelGenerator(
    const DisplayListSettings& settings,
    const StaticGeneratorOutput& staticOutput,
    const CollisionGeneratorOutput& collisionOutput
) : mSettings(settings), mStaticOutput(staticOutput), mCollisionOutput(collisionOutput) {}


int levelEdgeKey(int a, int b) {
    return (std::max(a, b) << 8) | std::min(a, b);
}

struct EdgeIndices {
    uint8_t a;
    uint8_t b;
};

std::unique_ptr<StructureDataChunk> LevelGenerator::CalculatePortalSingleSurface(CFileDefinition& fileDefinition, CollisionQuad& quad, ExtendedMesh& mesh, float scale) {
    std::unique_ptr<StructureDataChunk> portalSurface(new StructureDataChunk());

    std::unique_ptr<StructureDataChunk> vertices(new StructureDataChunk());

    for (unsigned i = 0; i < mesh.mMesh->mNumVertices; ++i) {
        short x, y;
        quad.ToLocalCoords(mesh.mMesh->mVertices[i] * scale, x, y);

        std::unique_ptr<StructureDataChunk> vertex(new StructureDataChunk());
        vertex->AddPrimitive(x);
        vertex->AddPrimitive(y);
        vertices->Add(std::move(vertex));
    }

    std::string meshName(mesh.mMesh->mName.C_Str());
    std::string verticesName = fileDefinition.GetUniqueName(meshName + "_portal_mesh");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct Vector2s16", verticesName, true, "_geo", std::move(vertices))));

    std::map<int, int> edgeUseCount;
    std::map<int, EdgeIndices> edgeDirection;
    std::vector<int> edgeOrder;

    for (unsigned faceIndex = 0; faceIndex < mesh.mMesh->mNumFaces; ++faceIndex) {
        aiFace* face = &mesh.mMesh->mFaces[faceIndex];

        for (unsigned index = 0; index < face->mNumIndices; ++index) {
            unsigned currentIndex = face->mIndices[index];
            unsigned nextIndex = face->mIndices[(index + 1) % face->mNumIndices];

            int key = levelEdgeKey(currentIndex, nextIndex);

            if (edgeUseCount.find(key) == edgeUseCount.end()) {
                edgeUseCount.insert(std::make_pair(key, 1));
                EdgeIndices indices = {(uint8_t)currentIndex, (uint8_t)nextIndex};
                edgeDirection.insert(std::make_pair(key, indices));
                edgeOrder.push_back(key);
            } else {
                edgeUseCount[key] = edgeUseCount[key] + 1;
            }
        }
    }

    // loops go first
    std::sort(edgeOrder.begin(), edgeOrder.end(), [&](int a, int b) -> bool {
        return edgeUseCount[a] < edgeUseCount[b];
    });

    int sideCount = 0;

    std::unique_ptr<StructureDataChunk> edges(new StructureDataChunk());

    for (auto key : edgeOrder) {
        if (edgeUseCount[key] == 1) {
            ++sideCount;
        }

        std::unique_ptr<StructureDataChunk> edge(new StructureDataChunk());
        EdgeIndices indices = edgeDirection[key];
        edge->AddPrimitive((int)indices.a);
        edge->AddPrimitive((int)indices.b);
        edges->Add(std::move(edge));
    }

    std::string edgesName = fileDefinition.GetUniqueName(meshName + "_portal_edges");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct SurfaceEdge", edgesName, true, "_geo", std::move(edges))));

    // vertices
    portalSurface->AddPrimitive(verticesName);
    // edges
    portalSurface->AddPrimitive(edgesName);
    // triangles
    portalSurface->AddPrimitive<const char*>("NULL");

    // sideCount
    portalSurface->AddPrimitive(sideCount);
    // edgesCount
    portalSurface->AddPrimitive(edgeOrder.size());
    // triangleCount
    portalSurface->AddPrimitive(0);

    portalSurface->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(quad.edgeA)));
    portalSurface->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(quad.edgeB)));
    portalSurface->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(quad.corner)));

    return portalSurface;
}

int LevelGenerator::CalculatePortalSurfaces(const aiScene* scene, CFileDefinition& fileDefinition, std::string& surfacesName, std::string& surfaceMappingName) {
    int surfaceCount = 0;

    std::unique_ptr<StructureDataChunk> portalSurfaceIndices(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> portalSurfaces(new StructureDataChunk());

    for (auto& collision : mCollisionOutput.quads) {
        int startSurfaceCount = surfaceCount;

        for (auto mesh : mStaticOutput.staticMeshes) {
            aiMaterial* material = scene->mMaterials[mesh->mMesh->mMaterialIndex];

            if (gPortalableSurfaces.find(ExtendedMesh::GetMaterialName(material)) == gPortalableSurfaces.end()) {
                continue;;
            }

            if (collision.IsCoplanar(*mesh, mSettings.mCollisionScale)) {
                portalSurfaces->Add(std::move(CalculatePortalSingleSurface(fileDefinition, collision, *mesh, mSettings.mCollisionScale)));                
                ++surfaceCount;
            }
        }

        std::unique_ptr<StructureDataChunk> indices(new StructureDataChunk());
        indices->AddPrimitive(startSurfaceCount);
        indices->AddPrimitive(surfaceCount);
        portalSurfaceIndices->Add(std::move(indices));
    }

    surfacesName = fileDefinition.GetUniqueName("portal_surfaces");
    std::unique_ptr<FileDefinition> portalSurfacesDef(new DataFileDefinition("struct PortalSurface", surfacesName, true, "_geo", std::move(portalSurfaces)));
    portalSurfacesDef->AddTypeHeader("\"scene/portal_surface.h\"");
    fileDefinition.AddDefinition(std::move(portalSurfacesDef));

    surfaceMappingName = fileDefinition.GetUniqueName("collider_to_surface");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct PortalSurfaceMapping", surfaceMappingName, true, "_geo", std::move(portalSurfaceIndices))));

    return surfaceCount;
}

void LevelGenerator::CalculateBoundingBoxes(const aiScene* scene, CFileDefinition& fileDefinition, std::string& boundingBoxesName) {
    std::unique_ptr<StructureDataChunk> boundingBoxes(new StructureDataChunk());

    for (auto& mesh : mStaticOutput.staticMeshes) {
        std::unique_ptr<StructureDataChunk> sphere(new StructureDataChunk());

        sphere->AddPrimitive((short)(mesh->bbMin.x * mSettings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMin.y * mSettings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMin.z * mSettings.mGraphicsScale + 0.5f));
        
        sphere->AddPrimitive((short)(mesh->bbMax.x * mSettings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMax.y * mSettings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMax.z * mSettings.mGraphicsScale + 0.5f));

        boundingBoxes->Add(std::move(sphere));
    }

    boundingBoxesName = fileDefinition.GetUniqueName("bounding_boxes");
    std::unique_ptr<FileDefinition> boundingBoxDef(new DataFileDefinition("struct BoundingBoxs16", boundingBoxesName, true, "_geo", std::move(boundingBoxes)));
    fileDefinition.AddDefinition(std::move(boundingBoxDef));
}

void LevelGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::string portalSurfaces;
    std::string portalSurfaceMapping;
    int portalSurfacesCount = CalculatePortalSurfaces(scene, fileDefinition, portalSurfaces, portalSurfaceMapping);

    std::string boundingBoxes;
    CalculateBoundingBoxes(scene, fileDefinition, boundingBoxes);
    
    std::unique_ptr<StructureDataChunk> levelDef(new StructureDataChunk());

    levelDef->AddPrimitive(mCollisionOutput.quadsName);
    levelDef->AddPrimitive(mStaticOutput.staticContentName);
    levelDef->AddPrimitive(boundingBoxes);
    levelDef->AddPrimitive(portalSurfaces);
    levelDef->AddPrimitive(portalSurfaceMapping);
    levelDef->AddPrimitive(mCollisionOutput.quads.size());
    levelDef->AddPrimitive(mStaticOutput.staticMeshes.size());
    levelDef->AddPrimitive(portalSurfacesCount);

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct LevelDefinition", fileDefinition.GetUniqueName("level"), false, "_geo", std::move(levelDef))));
}