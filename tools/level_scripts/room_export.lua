
local sk_scene = require('sk_scene')
local sk_math = require('sk_math')

local room_blocks = {}

for _, room in pairs(sk_scene.nodes_for_type("@room")) do
    local firstMesh = room.node.meshes[1]:transform(room.node.full_transformation)

    table.insert(room_blocks, {
        bb = firstMesh.bb,
        room_index = tonumber(room.arguments[1]),
    })
end

local function nearest_room_index(from_point, ignore_room)
    local result = nil
    local distance = nil

    for _, block in pairs(room_blocks) do
        if (block.room_index ~= ignore_room) then
            local distance_to_room = block.bb:distance_to_point(from_point)
            
            if (result == nil or distance_to_room < distance) then
                result = block.room_index
                distance = distance_to_room
            end
        end
    end 

    return result
end

local function node_nearest_room_index(from_node, ignore_room)
    local first_mesh = from_node.meshes[1]

    local local_center = sk_math.vector3(0, 0, 0) 
    
    if (first_mesh) then
        local_center = from_node.meshes[1].bb:lerp(0.5)
    end

    return nearest_room_index(from_node.full_transformation * local_center, ignore_room)
end

return {
    nearest_room_index = nearest_room_index,
    node_nearest_room_index = node_nearest_room_index,
}