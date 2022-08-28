#include "LevelGenerator.h"

#include <set>
#include <string>
#include "../math/MES.h"
#include "../MathUtl.h"
#include "./DecorGenerator.h"

std::set<std::string> gPortalableSurfaces = {
    "concrete_modular_wall001d",
    "concrete_modular_ceiling001a",
    "concrete_modular_floor001a",
};

int levelEdgeKey(int a, int b) {
    return (std::max(a, b) << 8) | std::min(a, b);
}

struct EdgeIndices {
    EdgeIndices();
    uint8_t a;
    uint8_t b;
    int edgeIndex;

    int nextEdgeKey;
    int prevEdgeKey;
    int nextEdgeReverseKey;
    int prevEdgeReverseKey;
};

EdgeIndices::EdgeIndices() :
    a(0), b(0),
    edgeIndex(-1),
    nextEdgeKey(-1),
    prevEdgeKey(-1),
    nextEdgeReverseKey(-1),
    prevEdgeReverseKey(-1) {

    }

uint8_t getEdgeIndex(std::map<int, EdgeIndices>& edges, int edgeKey) {
    auto result = edges.find(edgeKey);

    if (result == edges.end()) {
        return 0xFF;
    }

    return result->second.edgeIndex;
}

#define FIXED_POINT_PRECISION   8
#define FIXED_POINT_SCALAR      (1 << FIXED_POINT_PRECISION)

void toLocalCoords(const aiVector3D& corner, const aiVector3D& edgeA, const aiVector3D& edgeB, const aiVector3D& input, short& outX, short& outY) {
    aiVector3D relative = input - corner;

    outX = (short)(relative * edgeA * FIXED_POINT_SCALAR + 0.5f);
    outY = (short)(relative * edgeB * FIXED_POINT_SCALAR + 0.5f);
}

std::unique_ptr<StructureDataChunk> calculatePortalSingleSurface(CFileDefinition& fileDefinition, StaticMeshInfo& mesh, float scale) {
    std::unique_ptr<StructureDataChunk> portalSurface(new StructureDataChunk());

    std::unique_ptr<StructureDataChunk> vertices(new StructureDataChunk());

    std::string name(mesh.staticMesh->mMesh->mName.C_Str());

    aiVector3D origin = (mesh.staticMesh->bbMin + mesh.staticMesh->bbMax) * 0.5f * scale;

    aiVector3D normal;

    for (unsigned i = 0; i < mesh.staticMesh->mMesh->mNumVertices; ++i) {
        normal = normal + mesh.staticMesh->mMesh->mNormals[i];
    }

    normal.Normalize();
    aiVector3D right;
    aiVector3D up;

    if (fabsf(normal.z) < 0.7f) {
        right = aiVector3D(0.0f, 0.0f, 1.0f) ^ normal;
        right.Normalize();
        up = normal ^ right;
    } else {
        right = aiVector3D(1.0f, 0.0f, 0.0f) ^ normal;
        right.Normalize();
        up = normal ^ right;
    }

    for (unsigned i = 0; i < mesh.staticMesh->mMesh->mNumVertices; ++i) {
        short x, y;

        toLocalCoords(origin, right, up, mesh.staticMesh->mMesh->mVertices[i] * scale, x, y);

        std::unique_ptr<StructureDataChunk> vertexWrapperWrapper(new StructureDataChunk());
        std::unique_ptr<StructureDataChunk> vertexWrapper(new StructureDataChunk());
        std::unique_ptr<StructureDataChunk> vertex(new StructureDataChunk());
        vertex->AddPrimitive(x);
        vertex->AddPrimitive(y);
        vertexWrapper->Add(std::move(vertex));
        vertexWrapperWrapper->Add(std::move(vertexWrapper));
        vertices->Add(std::move(vertexWrapperWrapper));
    }

    std::string meshName(mesh.staticMesh->mMesh->mName.C_Str());
    std::string verticesName = fileDefinition.GetUniqueName(meshName + "_portal_mesh");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct Vector2s16", verticesName, true, "_geo", std::move(vertices))));

    std::map<int, int> edgeUseCount;
    std::map<int, EdgeIndices> edgeDirection;
    std::vector<int> edgeOrder;

    for (unsigned faceIndex = 0; faceIndex < mesh.staticMesh->mMesh->mNumFaces; ++faceIndex) {
        aiFace* face = &mesh.staticMesh->mMesh->mFaces[faceIndex];

        std::vector<int> edgeKeys;
        std::vector<bool> isReverseEdge;

        for (unsigned index = 0; index < face->mNumIndices; ++index) {
            unsigned currentIndex = face->mIndices[index];
            unsigned nextIndex = face->mIndices[(index + 1) % face->mNumIndices];

            int key = levelEdgeKey(currentIndex, nextIndex);

            edgeKeys.push_back(key);

            if (edgeUseCount.find(key) == edgeUseCount.end()) {
                edgeUseCount.insert(std::make_pair(key, 1));
                EdgeIndices indices;
                indices.a = currentIndex;
                indices.b = nextIndex;
                edgeDirection.insert(std::make_pair(key, indices));
                isReverseEdge.push_back(false);
                edgeOrder.push_back(key);
            } else {
                edgeUseCount[key] = edgeUseCount[key] + 1;
                isReverseEdge.push_back(true);
            }
        }

        // connect faces in a loop
        for (int i = 0; i < (int)edgeKeys.size(); ++i) {
            int prevKey = edgeKeys[(i + edgeKeys.size() - 1) % edgeKeys.size()];
            int nextKey = edgeKeys[(i + 1) % edgeKeys.size()];

            EdgeIndices& edge = edgeDirection[edgeKeys[i]];

            if (isReverseEdge[i]) {
                edge.nextEdgeReverseKey = nextKey;
                edge.prevEdgeReverseKey = prevKey;
            } else {
                edge.nextEdgeKey = nextKey;
                edge.prevEdgeKey = prevKey;
            }
        }
    }

    // edges go first
    std::sort(edgeOrder.begin(), edgeOrder.end(), [&](int a, int b) -> bool {
        int edgeDiff = edgeUseCount[a] - edgeUseCount[b];

        if (edgeDiff != 0) {
            return edgeDiff < 0;
        }

        // this is an attempt to make edges that are near each other
        // show up in memory near each other to improve cache
        // performance
        return edgeDirection[a].a < edgeDirection[b].a;
    });

    int edgeIndex = 0;

    for (auto key : edgeOrder) {
        edgeDirection[key].edgeIndex = edgeIndex;
        ++edgeIndex;
    }

    std::unique_ptr<StructureDataChunk> edges(new StructureDataChunk());

    for (auto key : edgeOrder) {

        std::unique_ptr<StructureDataChunk> edge(new StructureDataChunk());
        EdgeIndices indices = edgeDirection[key];
        edge->AddPrimitive((int)indices.a);
        edge->AddPrimitive((int)indices.b);
        edge->AddPrimitive((int)getEdgeIndex(edgeDirection, indices.nextEdgeKey));
        edge->AddPrimitive((int)getEdgeIndex(edgeDirection, indices.prevEdgeKey));
        edge->AddPrimitive((int)getEdgeIndex(edgeDirection, indices.nextEdgeReverseKey));
        edge->AddPrimitive((int)getEdgeIndex(edgeDirection, indices.prevEdgeReverseKey));
        edges->Add(std::move(edge));
    }

    std::string edgesName = fileDefinition.GetUniqueName(meshName + "_portal_edges");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct SurfaceEdge", edgesName, true, "_geo", std::move(edges))));

    // vertices
    portalSurface->AddPrimitive(verticesName);
    // edges
    portalSurface->AddPrimitive(edgesName);

    // edgesCount
    portalSurface->AddPrimitive(edgeOrder.size());
    // vertexCount
    portalSurface->AddPrimitive(mesh.staticMesh->mMesh->mNumVertices);
    // shouldCleanup
    portalSurface->AddPrimitive(0);

    portalSurface->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(right)));
    portalSurface->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(up)));
    portalSurface->Add(std::unique_ptr<DataChunk>(new StructureDataChunk(origin)));

    std::string vertexBufferName = fileDefinition.GetVertexBuffer(
        mesh.staticMesh, 
        Material::GetVertexType(mesh.material), 
        Material::TextureWidth(mesh.material), 
        Material::TextureHeight(mesh.material), 
        "_geo"
    );

    portalSurface->AddPrimitive(vertexBufferName);
    portalSurface->AddPrimitive(mesh.gfxName);
    
    return portalSurface;
}

void generatePortalSurfacesDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const CollisionGeneratorOutput& collisionOutput, const StaticGeneratorOutput& staticOutput, const DisplayListSettings& settings) {
    int surfaceCount = 0;

    std::unique_ptr<StructureDataChunk> portalMappingRange(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> portalSurfaces(new StructureDataChunk());

    std::vector<int> staticToPortableSurfaceMapping;

    for (auto mesh : staticOutput.staticMeshes) {
        aiMaterial* material = scene->mMaterials[mesh.staticMesh->mMesh->mMaterialIndex];

        std::string materialName = ExtendedMesh::GetMaterialName(material, settings.mForceMaterialName);

        if (gPortalableSurfaces.find(materialName) == gPortalableSurfaces.end()) {
            staticToPortableSurfaceMapping.push_back(-1);
            continue;
        }

        portalSurfaces->Add(std::move(calculatePortalSingleSurface(fileDefinition, mesh, settings.mModelScale)));
        staticToPortableSurfaceMapping.push_back(surfaceCount);
        ++surfaceCount;
    }

    int mappingIndex = 0;
    std::unique_ptr<StructureDataChunk> portalMappingData(new StructureDataChunk());

    for (auto& collision : collisionOutput.quads) {
        int startMappingIndex = mappingIndex;

        aiAABB collisionWithPadding = collision.BoundingBox();
        collisionWithPadding.mMin = collisionWithPadding.mMin - aiVector3D(0.1f, 0.1f, 0.1f);
        collisionWithPadding.mMax = collisionWithPadding.mMax + aiVector3D(0.1f, 0.1f, 0.1f);

        for (std::size_t staticMeshIndex = 0; staticMeshIndex < staticOutput.staticMeshes.size(); ++staticMeshIndex) {
            const StaticMeshInfo& mesh = staticOutput.staticMeshes[staticMeshIndex];

            aiMaterial* material = scene->mMaterials[mesh.staticMesh->mMesh->mMaterialIndex];

            std::string materialName = ExtendedMesh::GetMaterialName(material, settings.mForceMaterialName);

            if (gPortalableSurfaces.find(materialName) == gPortalableSurfaces.end()) {
                continue;
            }

            aiAABB meshBB(mesh.staticMesh->bbMin * settings.mModelScale, mesh.staticMesh->bbMax * settings.mModelScale);

            if (!collision.IsCoplanar(*mesh.staticMesh, settings.mModelScale)) {
                continue;
            }

            if (!doesAABBOverlap(collisionWithPadding, meshBB)) {
                continue;
            }

            portalMappingData->AddPrimitive(staticToPortableSurfaceMapping[staticMeshIndex]);
            ++mappingIndex;
        }

        if (mappingIndex > 255) {
            std::cerr << "Mapping index larger than 255" << std::endl;
            exit(1);
        }

        std::unique_ptr<StructureDataChunk> indices(new StructureDataChunk());
        indices->AddPrimitive(startMappingIndex);
        indices->AddPrimitive(mappingIndex);
        portalMappingRange->Add(std::move(indices));
    }

    std::string surfacesName = fileDefinition.GetUniqueName("portal_surfaces");
    std::unique_ptr<FileDefinition> portalSurfacesDef(new DataFileDefinition("struct PortalSurface", surfacesName, true, "_geo", std::move(portalSurfaces)));
    portalSurfacesDef->AddTypeHeader("\"scene/portal_surface.h\"");
    fileDefinition.AddDefinition(std::move(portalSurfacesDef));

    std::string surfaceMappingName = fileDefinition.GetUniqueName("collider_to_surface");
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct PortalSurfaceMappingRange", surfaceMappingName, true, "_geo", std::move(portalMappingRange))));

    std::string portalSurfaceMappingIndices = fileDefinition.AddDataDefinition(
        "mapping_indices",
        "u8",
        true,
        "_geo",
        std::move(portalMappingData)
    );

    levelDef.AddPrimitive("portalSurfaces", surfacesName);
    levelDef.AddPrimitive("portalSurfaceMappingRange", surfaceMappingName);
    levelDef.AddPrimitive("portalSurfaceMappingIndices", portalSurfaceMappingIndices);
    levelDef.AddPrimitive("portalSurfaceCount", surfaceCount);
}

void generateBoundingBoxesDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const StaticGeneratorOutput& staticOutput, const DisplayListSettings& settings) {
    std::unique_ptr<StructureDataChunk> boundingBoxes(new StructureDataChunk());

    float combinedScale = settings.mFixedPointScale * settings.mModelScale;

    for (auto& mesh : staticOutput.staticMeshes) {
        std::unique_ptr<StructureDataChunk> sphere(new StructureDataChunk());

        sphere->AddPrimitive((short)(mesh.staticMesh->bbMin.x * combinedScale + 0.5f));
        sphere->AddPrimitive((short)(mesh.staticMesh->bbMin.y * combinedScale + 0.5f));
        sphere->AddPrimitive((short)(mesh.staticMesh->bbMin.z * combinedScale + 0.5f));
        
        sphere->AddPrimitive((short)(mesh.staticMesh->bbMax.x * combinedScale + 0.5f));
        sphere->AddPrimitive((short)(mesh.staticMesh->bbMax.y * combinedScale + 0.5f));
        sphere->AddPrimitive((short)(mesh.staticMesh->bbMax.z * combinedScale + 0.5f));

        boundingBoxes->Add(std::move(sphere));
    }

    std::string boundingBoxesName = fileDefinition.GetUniqueName("bounding_boxes");
    std::unique_ptr<FileDefinition> boundingBoxDef(new DataFileDefinition("struct BoundingBoxs16", boundingBoxesName, true, "_geo", std::move(boundingBoxes)));
    fileDefinition.AddDefinition(std::move(boundingBoxDef));

    levelDef.AddPrimitive("staticBoundingBoxes", boundingBoxesName);
}

void generateTriggerDefinition(const aiScene* scene, CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const TriggerGeneratorOutput& triggerOutput) {
    std::unique_ptr<StructureDataChunk> triggers(new StructureDataChunk());

    int triggerCount = 0;

    for (auto& trigger : triggerOutput.triggers) {
        int cutsceneForTrigger = 0;

        for (auto& cutscene : triggerOutput.cutscenes) {
            if (cutscene->name == trigger->cutsceneName) {
                break;
            }
            ++cutsceneForTrigger;
        }

        if (cutsceneForTrigger == (int)triggerOutput.cutscenes.size()) {
            continue;
        }

        std::unique_ptr<StructureDataChunk> triggerData(new StructureDataChunk());

        triggerData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(trigger->bb)));
        triggerData->AddPrimitive(cutsceneForTrigger);

        triggers->Add(std::move(triggerData));
        ++triggerCount;
    }

    std::string triggersName = fileDefinition.GetUniqueName("triggers");
    std::unique_ptr<FileDefinition> triggersDef(new DataFileDefinition("struct Trigger", triggersName, true, "_geo", std::move(triggers)));
    fileDefinition.AddDefinition(std::move(triggersDef));

    levelDef.AddPrimitive("triggers", triggersName);
    levelDef.AddPrimitive("triggerCount", triggerCount);

    std::unique_ptr<StructureDataChunk> cutscenes(new StructureDataChunk());

    for (auto& cutscene : triggerOutput.cutscenes) {
        std::unique_ptr<StructureDataChunk> cutsceneDef(new StructureDataChunk());
       
        cutsceneDef->AddPrimitive((cutscene->stepsDefName == "") ? std::string("NULL") : cutscene->stepsDefName);
        cutsceneDef->AddPrimitive(cutscene->steps.size());

        cutscenes->Add(std::move(cutsceneDef));
    }

    std::string cutscenesName = fileDefinition.AddDataDefinition("cutscenes", "struct Cutscene", true, "_geo", std::move(cutscenes));

    levelDef.AddPrimitive("cutscenes", cutscenesName);
    levelDef.AddPrimitive("cutsceneCount", triggerOutput.cutscenes.size());
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

void generateFizzlerDefinitions(
    const aiScene* scene, 
    CFileDefinition& fileDefinition, 
    StructureDataChunk& levelDef, 
    const RoomGeneratorOutput& roomOutput, 
    const DisplayListSettings& settings,
    NodeGroups& nodeGroups) {
    
    int fizzlerCount = 0;
    std::unique_ptr<StructureDataChunk> fizzlers(new StructureDataChunk());

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    for (auto& nodeInfo : nodeGroups.NodesForType("@fizzler")) {
        std::unique_ptr<StructureDataChunk> fizzlerData(new StructureDataChunk());
        aiVector3D pos;
        aiQuaternion rot;
        aiVector3D scale;
        (baseTransform * nodeInfo.node->mTransformation).Decompose(scale, rot, pos);
        fizzlerData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(pos)));
        fizzlerData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(rot)));
        fizzlerData->AddPrimitive(2.0f);
        fizzlerData->AddPrimitive(2.0f);
        fizzlerData->AddPrimitive(roomOutput.RoomForNode(nodeInfo.node));

        fizzlers->Add(std::move(fizzlerData));
        ++fizzlerCount;
    }

    std::string fizzlersName = fileDefinition.AddDataDefinition(
        "fizzlers",
        "struct FizzlerDefinition",
        true,
        "_geo",
        std::move(fizzlers)
    );

    levelDef.AddPrimitive("fizzlers", fizzlersName);
    levelDef.AddPrimitive("fizzlerCount", fizzlerCount);
}

void generateElevatorDefinitions(
    const aiScene* scene, 
    CFileDefinition& fileDefinition, 
    StructureDataChunk& levelDef, 
    const RoomGeneratorOutput& roomOutput, 
    const DisplayListSettings& settings,
    Signals& signals,
    NodeGroups& nodeGroups) {

    int elevatorCount = 0;
    std::unique_ptr<StructureDataChunk> elevators(new StructureDataChunk());

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    auto nodes = nodeGroups.NodesForType("@elevator");

    for (auto& nodeInfo : nodes) {
        if (nodeInfo.arguments.size() == 0) {
            continue;
        }

        std::unique_ptr<StructureDataChunk> elevatorData(new StructureDataChunk());
        aiVector3D pos;
        aiQuaternion rot;
        aiVector3D scale;
        (baseTransform * nodeInfo.node->mTransformation).Decompose(scale, rot, pos);
        elevatorData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(pos)));
        elevatorData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(rot)));
        elevatorData->AddPrimitive(roomOutput.RoomForNode(nodeInfo.node));

        if (nodeInfo.arguments.size() > 1) {
            std::string targetElevator = nodeInfo.arguments[1];

            if (targetElevator == "next_level") {
                elevatorData->AddPrimitive(nodes.size());
            } else {
                unsigned targetIndex = 0;
                for (targetIndex = 0; targetIndex < nodes.size(); ++targetIndex) {
                    if (nodes[targetIndex].arguments.size() && nodes[targetIndex].arguments[0] == targetElevator) {
                        break;
                    }
                }

                elevatorData->AddPrimitive(targetIndex);
            }
        } else {
            elevatorData->AddPrimitive(-1);
        }

        elevators->Add(std::move(elevatorData));
        ++elevatorCount;
    }

    std::string elevatorsName = fileDefinition.AddDataDefinition(
        "elevators",
        "struct ElevatorDefinition",
        true,
        "_geo",
        std::move(elevators)
    );

    levelDef.AddPrimitive("elevators", elevatorsName);
    levelDef.AddPrimitive("elevatorCount", elevatorCount);
}

void generatePedestalDefinitions(
    const aiScene* scene, 
    CFileDefinition& fileDefinition, 
    StructureDataChunk& levelDef, 
    const RoomGeneratorOutput& roomOutput, 
    const DisplayListSettings& settings,
    NodeGroups& nodeGroups) {

    int pedestalCount = 0;
    std::unique_ptr<StructureDataChunk> pedestals(new StructureDataChunk());

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    for (auto& nodeInfo : nodeGroups.NodesForType("@pedestal")) {
        std::unique_ptr<StructureDataChunk> pedestalsData(new StructureDataChunk());
        aiVector3D pos;
        aiQuaternion rot;
        aiVector3D scale;
        (baseTransform * nodeInfo.node->mTransformation).Decompose(scale, rot, pos);
        pedestalsData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(pos)));
        pedestalsData->AddPrimitive(roomOutput.RoomForNode(nodeInfo.node));

        pedestals->Add(std::move(pedestalsData));
        ++pedestalCount;
    }

    std::string pedestalsCount = fileDefinition.AddDataDefinition(
        "pedestals",
        "struct PedestalDefinition",
        true,
        "_geo",
        std::move(pedestals)
    );

    levelDef.AddPrimitive("pedestals", pedestalsCount);
    levelDef.AddPrimitive("pedestalCount", pedestalCount);
}


void generateSignageDefinitions(
    const aiScene* scene, 
    CFileDefinition& fileDefinition, 
    StructureDataChunk& levelDef, 
    const RoomGeneratorOutput& roomOutput, 
    const DisplayListSettings& settings,
    NodeGroups& nodeGroups) {

    int signageCount = 0;
    std::unique_ptr<StructureDataChunk> signage(new StructureDataChunk());

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    for (auto& nodeInfo : nodeGroups.NodesForType("@signage")) {
        if (nodeInfo.arguments.size() == 0) {
            continue;
        }

        std::unique_ptr<StructureDataChunk> signageData(new StructureDataChunk());
        aiVector3D pos;
        aiQuaternion rot;
        aiVector3D scale;
        (baseTransform * nodeInfo.node->mTransformation).Decompose(scale, rot, pos);
        signageData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(pos)));
        signageData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(rot)));
        signageData->AddPrimitive(roomOutput.RoomForNode(nodeInfo.node));
        signageData->AddPrimitive(std::atoi(nodeInfo.arguments[0].c_str()));

        signage->Add(std::move(signageData));
        ++signageCount;
    }

    std::string signageDef = fileDefinition.AddDataDefinition(
        "signage",
        "struct SignageDefinition",
        true,
        "_geo",
        std::move(signage)
    );

    levelDef.AddPrimitive("signage", signageDef);
    levelDef.AddPrimitive("signageCount", signageCount);
}


void generateBoxDropperDefinitions(
    const aiScene* scene, 
    CFileDefinition& fileDefinition, 
    StructureDataChunk& levelDef, 
    const RoomGeneratorOutput& roomOutput, 
    const DisplayListSettings& settings,
    NodeGroups& nodeGroups,
    Signals& signals) {

    int boxDropperCount = 0;
    std::unique_ptr<StructureDataChunk> signage(new StructureDataChunk());

    aiMatrix4x4 baseTransform = settings.CreateCollisionTransform();

    for (auto& nodeInfo : nodeGroups.NodesForType("@box_dropper")) {
        if (nodeInfo.arguments.size() == 0) {
            continue;
        }

        std::unique_ptr<StructureDataChunk> boxDropperData(new StructureDataChunk());
        aiVector3D pos;
        aiQuaternion rot;
        aiVector3D scale;
        (baseTransform * nodeInfo.node->mTransformation).Decompose(scale, rot, pos);
        boxDropperData->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(pos)));
        boxDropperData->AddPrimitive(roomOutput.RoomForNode(nodeInfo.node));
        boxDropperData->AddPrimitive(signals.SignalIndexForName(nodeInfo.arguments[0]));

        signage->Add(std::move(boxDropperData));
        ++boxDropperCount;
    }

    std::string boxDropperDef = fileDefinition.AddDataDefinition(
        "box_dropper",
        "struct BoxDropperDefinition",
        true,
        "_geo",
        std::move(signage)
    );

    levelDef.AddPrimitive("boxDroppers", boxDropperDef);
    levelDef.AddPrimitive("boxDropperCount", boxDropperCount);
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
        Signals& signals,
        NodeGroups& nodeGroups
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

    generateDecorDefinition(scene, fileDefinition, *levelDef, roomOutput, settings, nodeGroups);

    generateFizzlerDefinitions(scene, fileDefinition, *levelDef, roomOutput, settings, nodeGroups);

    generateElevatorDefinitions(scene, fileDefinition, *levelDef, roomOutput, settings, signals, nodeGroups);

    generatePedestalDefinitions(scene, fileDefinition, *levelDef, roomOutput, settings, nodeGroups);

    generateSignageDefinitions(scene, fileDefinition, *levelDef, roomOutput, settings, nodeGroups);

    generateBoxDropperDefinitions(scene, fileDefinition, *levelDef, roomOutput, settings, nodeGroups, signals);

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("struct LevelDefinition", fileDefinition.GetUniqueName("level"), false, "_geo", std::move(levelDef))));
}