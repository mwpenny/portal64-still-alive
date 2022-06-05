#include "LevelGenerator.h"

#include <set>
#include <string>
#include "../math/MES.h"
#include "../MathUtl.h"

std::set<std::string> gPortalableSurfaces = {
    "concrete_modular_wall001d",
    "concrete_modular_ceiling001a",
    "concrete_modular_floor001a",
};

int levelEdgeKey(int a, int b) {
    return (std::max(a, b) << 8) | std::min(a, b);
}

struct EdgeIndices {
    uint8_t a;
    uint8_t b;
};

std::unique_ptr<StructureDataChunk> calculatePortalSingleSurface(CFileDefinition& fileDefinition, const CollisionQuad& quad, ExtendedMesh& mesh, float scale) {
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

void generatePortalSurfacesDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const CollisionGeneratorOutput& collisionOutput, const StaticGeneratorOutput& staticOutput, const DisplayListSettings& settings) {
    int surfaceCount = 0;

    std::unique_ptr<StructureDataChunk> portalSurfaceIndices(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> portalSurfaces(new StructureDataChunk());

    for (auto& collision : collisionOutput.quads) {
        int startSurfaceCount = surfaceCount;

        aiAABB collisionWithPadding = collision.BoundingBox();
        collisionWithPadding.mMin = collisionWithPadding.mMin - aiVector3D(0.1f, 0.1f, 0.1f);
        collisionWithPadding.mMax = collisionWithPadding.mMax + aiVector3D(0.1f, 0.1f, 0.1f);

        for (auto mesh : staticOutput.staticMeshes) {
            aiMaterial* material = scene->mMaterials[mesh->mMesh->mMaterialIndex];

            std::string materialName = ExtendedMesh::GetMaterialName(material);

            if (gPortalableSurfaces.find(materialName) == gPortalableSurfaces.end()) {
                continue;
            }

            aiAABB meshBB(mesh->bbMin * settings.mCollisionScale, mesh->bbMax * settings.mCollisionScale);

            if (!collision.IsCoplanar(*mesh, settings.mCollisionScale)) {
                continue;
            }

            if (!doesAABBOverlap(collisionWithPadding, meshBB)) {
                continue;
            }

            portalSurfaces->Add(std::move(calculatePortalSingleSurface(fileDefinition, collision, *mesh, settings.mCollisionScale)));                
            ++surfaceCount;
        }

        std::unique_ptr<StructureDataChunk> indices(new StructureDataChunk());
        indices->AddPrimitive(startSurfaceCount);
        indices->AddPrimitive(surfaceCount);
        portalSurfaceIndices->Add(std::move(indices));
    }

    std::string surfacesName = fileDefinition.GetUniqueName("portal_surfaces");
    std::unique_ptr<FileDefinition> portalSurfacesDef(new DataFileDefinition("struct PortalSurface", surfacesName, true, "_geo", std::move(portalSurfaces)));
    portalSurfacesDef->AddTypeHeader("\"scene/portal_surface.h\"");
    fileDefinition.AddDefinition(std::move(portalSurfacesDef));

    std::string surfaceMappingName = fileDefinition.GetUniqueName("collider_to_surface");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct PortalSurfaceMapping", surfaceMappingName, true, "_geo", std::move(portalSurfaceIndices))));

    levelDef.AddPrimitive("portalSurfaces", surfacesName);
    levelDef.AddPrimitive("portalSurfaceMapping", surfaceMappingName);
    levelDef.AddPrimitive("portalSurfaceCount", surfaceCount);
}

void generateBoundingBoxesDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const StaticGeneratorOutput& staticOutput, const DisplayListSettings& settings) {
    std::unique_ptr<StructureDataChunk> boundingBoxes(new StructureDataChunk());

    for (auto& mesh : staticOutput.staticMeshes) {
        std::unique_ptr<StructureDataChunk> sphere(new StructureDataChunk());

        sphere->AddPrimitive((short)(mesh->bbMin.x * settings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMin.y * settings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMin.z * settings.mGraphicsScale + 0.5f));
        
        sphere->AddPrimitive((short)(mesh->bbMax.x * settings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMax.y * settings.mGraphicsScale + 0.5f));
        sphere->AddPrimitive((short)(mesh->bbMax.z * settings.mGraphicsScale + 0.5f));

        boundingBoxes->Add(std::move(sphere));
    }

    std::string boundingBoxesName = fileDefinition.GetUniqueName("bounding_boxes");
    std::unique_ptr<FileDefinition> boundingBoxDef(new DataFileDefinition("struct BoundingBoxs16", boundingBoxesName, true, "_geo", std::move(boundingBoxes)));
    fileDefinition.AddDefinition(std::move(boundingBoxDef));

    levelDef.AddPrimitive("staticBoundingBoxes", boundingBoxesName);
}

void generateTriggerDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const TriggerGeneratorOutput& triggerOutput) {
    std::unique_ptr<StructureDataChunk> triggers(new StructureDataChunk());

    for (auto& trigger : triggerOutput.triggers) {
        std::unique_ptr<StructureDataChunk> triggerData(new StructureDataChunk());

        std::unique_ptr<StructureDataChunk> cutsceneDef(new StructureDataChunk());
       
        cutsceneDef->AddPrimitive((trigger->stepsName == "") ? std::string("NULL") : trigger->stepsName);
        cutsceneDef->AddPrimitive(trigger->stepsCount);

        triggerData->Add(std::move(cutsceneDef));
        triggerData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(trigger->bb)));

        triggers->Add(std::move(triggerData));
    }

    std::string triggersName = fileDefinition.GetUniqueName("triggers");
    std::unique_ptr<FileDefinition> triggersDef(new DataFileDefinition("struct Trigger", triggersName, true, "_geo", std::move(triggers)));
    fileDefinition.AddDefinition(std::move(triggersDef));

    levelDef.AddPrimitive("triggers", triggersName);
    levelDef.AddPrimitive("triggerCount", triggerOutput.triggers.size());
}

void generateLocationDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const RoomGeneratorOutput& roomOutput, const DisplayListSettings& settings) {
    aiMatrix4x4 baseTransfrom = settings.CreateCollisionTransform();

    std::unique_ptr<StructureDataChunk> locations(new StructureDataChunk());

    for (auto& location : roomOutput.namedLocations) {
        aiMatrix4x4 nodeTransform = baseTransfrom * location.node->mTransformation;

        aiVector3D position;
        aiQuaternion rotation;
        aiVector3D scale;
        nodeTransform.Decompose(scale, rotation, position);

        std::unique_ptr<StructureDataChunk> locationData(new StructureDataChunk());

        std::unique_ptr<StructureDataChunk> transform(new StructureDataChunk());
        transform->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(position)));
        transform->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(rotation)));
        transform->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(scale)));
        locationData->Add(std::move(transform));

        auto roomIndex = roomOutput.roomIndexMapping.find(location.node);

        locationData->AddPrimitive(roomIndex == roomOutput.roomIndexMapping.end() ? 0 : roomIndex->second);

        locations->Add(std::move(locationData));
    }

    std::string locationsName = fileDefinition.GetUniqueName("locations");
    std::unique_ptr<FileDefinition> triggersDef(new DataFileDefinition("struct Location", locationsName, true, "_geo", std::move(locations)));
    fileDefinition.AddDefinition(std::move(triggersDef));

    levelDef.AddPrimitive("locations", locationsName);
    levelDef.AddPrimitive("locationCount", roomOutput.namedLocations.size());
}

void generateWorldDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const RoomGeneratorOutput& roomOutput, const CollisionGeneratorOutput& collisionOutput) {
    std::vector<std::vector<int>> roomDoorways;

    for (int i = 0; i < roomOutput.roomCount; ++i) {
        roomDoorways.push_back(std::vector<int>());
    }

    int doorwayIndex = 0;

    std::unique_ptr<StructureDataChunk> doorways(new StructureDataChunk());

    for (auto& doorway : roomOutput.doorways) {
        roomDoorways[doorway.roomA].push_back(doorwayIndex);
        roomDoorways[doorway.roomB].push_back(doorwayIndex);

        std::unique_ptr<StructureDataChunk> doorwayData(new StructureDataChunk());
        doorwayData->Add(std::move(doorway.quad.Generate()));
        doorwayData->AddPrimitive(doorway.roomA);
        doorwayData->AddPrimitive(doorway.roomB);
        doorwayData->AddPrimitive<const char*>("DoorwayFlagsOpen");
        doorways->Add(std::move(doorwayData));

        ++doorwayIndex;
    }

    std::string doorwaysName = fileDefinition.AddDataDefinition(
        "doorways",
        "struct Doorway",
        true, 
        "_geo",
        std::move(doorways)
    );

    std::unique_ptr<StructureDataChunk> rooms(new StructureDataChunk());

    for (int i = 0; i < roomOutput.roomCount; ++i) {
        std::unique_ptr<StructureDataChunk> doorwayList(new StructureDataChunk());

        for (auto doorway : roomDoorways[i]) {
            doorwayList->AddPrimitive(doorway);
        }

        std::string doorwayListName = fileDefinition.AddDataDefinition(
            "room_doorways",
            "short",
            true, 
            "_geo",
            std::move(doorwayList)
        );

        std::unique_ptr<StructureDataChunk> quadIndices(new StructureDataChunk());
        std::unique_ptr<StructureDataChunk> cellContents(new StructureDataChunk());

        int startIndex = 0;

        for (auto& zRange : collisionOutput.roomGrids[i].cells) {
            for (auto& indices : zRange) {
                for (auto index : indices) {
                    quadIndices->AddPrimitive(index);
                }

                int endIndex = startIndex + indices.size();

                std::unique_ptr<StructureDataChunk> range(new StructureDataChunk());
                range->AddPrimitive(startIndex);
                range->AddPrimitive(endIndex);
                cellContents->Add(std::move(range));

                startIndex = endIndex;

            }
        }

        std::string quadIndicesName = fileDefinition.AddDataDefinition(
            "room_indices",
            "short",
            true,
            "_geo",
            std::move(quadIndices)
        );

        std::string cellContentsName = fileDefinition.AddDataDefinition(
            "room_cells",
            "struct Rangeu16",
            true,
            "_geo",
            std::move(cellContents)
        );

        std::unique_ptr<StructureDataChunk> room(new StructureDataChunk());
        room->AddPrimitive(std::move(quadIndicesName));
        room->AddPrimitive(std::move(cellContentsName));
        room->AddPrimitive(collisionOutput.roomGrids[i].spanX);
        room->AddPrimitive(collisionOutput.roomGrids[i].spanZ);
        room->AddPrimitive(collisionOutput.roomGrids[i].x);
        room->AddPrimitive(collisionOutput.roomGrids[i].z);
        room->AddPrimitive(std::move(doorwayListName));
        room->AddPrimitive(roomDoorways[i].size());
        rooms->Add(std::move(room));
    }

    std::string roomsName = fileDefinition.AddDataDefinition(
        "rooms",
        "struct Room",
        true, 
        "_geo",
        std::move(rooms)
    );


    std::unique_ptr<StructureDataChunk> worldDef(new StructureDataChunk());
    worldDef->AddPrimitive("rooms", roomsName);
    worldDef->AddPrimitive("doorways", doorwaysName);
    worldDef->AddPrimitive("roomCount", roomOutput.roomCount);
    worldDef->AddPrimitive("doorwayCount", roomOutput.doorways.size());
    levelDef.Add("world", std::move(worldDef));
}

void generateDoorsDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const RoomGeneratorOutput& roomOutput, const DisplayListSettings& settings) {
    std::unique_ptr<StructureDataChunk> doors(new StructureDataChunk());

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    for (auto& door : roomOutput.doors) {
        std::unique_ptr<StructureDataChunk> doorData(new StructureDataChunk());
        aiVector3D pos;
        aiQuaternion rot;
        aiVector3D scale;
        (baseTransform * door.node->mTransformation).Decompose(scale, rot, pos);
        doorData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(pos)));
        doorData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(rot)));
        doorData->AddPrimitive(door.doorwayIndex);
        doorData->AddPrimitive(door.signalIndex);
        doors->Add(std::move(doorData));
    }

    std::string doorsName = fileDefinition.AddDataDefinition(
        "doors",
        "struct DoorDefinition",
        true,
        "_geo",
        std::move(doors)
    );

    levelDef.AddPrimitive("doors", doorsName);
    levelDef.AddPrimitive("doorCount", roomOutput.doors.size());
}

void generateLevel(
        const aiScene* scene, 
        CFileDefinition& fileDefinition,
        const DisplayListSettings& settings,
        const StaticGeneratorOutput& staticOutput,
        const CollisionGeneratorOutput& collisionOutput,
        const TriggerGeneratorOutput& triggerOutput,
        const RoomGeneratorOutput& roomOutput,
        const SignalsOutput& signalsOutput,
        Signals& signals
) {    
    std::unique_ptr<StructureDataChunk> levelDef(new StructureDataChunk());

    levelDef->AddPrimitive("collisionQuads", collisionOutput.quadsName);
    levelDef->AddPrimitive("staticContent", staticOutput.staticContentName);
    levelDef->AddPrimitive("roomStaticMapping", staticOutput.roomMappingName);

    levelDef->AddPrimitive("collisionQuadCount", collisionOutput.quads.size());
    levelDef->AddPrimitive("staticContentCount", staticOutput.staticMeshes.size());
    levelDef->AddPrimitive("startLocation", roomOutput.FindLocationIndex("start"));

    generatePortalSurfacesDefinition(scene, fileDefinition, *levelDef, collisionOutput, staticOutput, settings);

    generateBoundingBoxesDefinition(scene, fileDefinition, *levelDef, staticOutput, settings);

    generateTriggerDefinition(scene, fileDefinition, *levelDef, triggerOutput);

    generateLocationDefinition(scene, fileDefinition, *levelDef, roomOutput, settings);

    generateWorldDefinition(scene, fileDefinition, *levelDef, roomOutput, collisionOutput);

    generateDoorsDefinition(scene, fileDefinition, *levelDef, roomOutput, settings);
    generateButtonsDefinition(fileDefinition, *levelDef, triggerOutput.buttons);

    generateSignalsDefinition(fileDefinition, *levelDef, signalsOutput, signals);

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct LevelDefinition", fileDefinition.GetUniqueName("level"), false, "_geo", std::move(levelDef))));
}