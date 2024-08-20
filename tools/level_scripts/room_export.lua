
local sk_scene = require('sk_scene')
local sk_math = require('sk_math')
local util = require('tools.level_scripts.util')

local room_blocks = {}
local room_count = 0
local room_bb = {}
local room_non_visibility = {}

for _, room in pairs(sk_scene.nodes_for_type("@room")) do
    local firstMesh = room.node.meshes[1]:transform(room.node.full_transformation)
    local room_index = tonumber(room.arguments[1])
    local non_visible_rooms = 0

    local can_see = sk_scene.find_named_argument(room.arguments, "can_see")
    if can_see then
        local visible_rooms = 1 << room_index

        local can_see_rooms = util.string_split(util.trim(can_see), ',')
        for _, can_see_index in pairs(can_see_rooms) do
            visible_rooms = visible_rooms | (1 << tonumber(can_see_index))
        end

        non_visible_rooms = ~visible_rooms
    end

    room_count = math.max(room_count, room_index + 1)

    if room_bb[room_index + 1] then
        room_bb[room_index + 1] = room_bb[room_index + 1]:union(firstMesh.bb)
    else
        room_bb[room_index + 1] = firstMesh.bb
    end

    if room_non_visibility[room_index + 1] then
        room_non_visibility[room_index + 1] = room_non_visibility[room_index + 1] | non_visible_rooms
    else
        room_non_visibility[room_index + 1] = non_visible_rooms
    end

    table.insert(room_blocks, {
        bb = firstMesh.bb,
        room_index = room_index,
    })
end

local function nearest_room_index(from_point, ignore_room)
    local result = nil
    local distance = nil

    for _, block in pairs(room_blocks) do
        if (block.room_index ~= ignore_room) then
            local distance_to_room = block.bb:distance_to_point(from_point)
            
            if (result == nil or distance_to_room < distance) then
                result = block
                distance = distance_to_room
            end
        end
    end 

    if result == nil then
        return nil, nil
    end

    return result.room_index, result.bb
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
    room_count = room_count,
    room_bb = room_bb,
    room_non_visibility = room_non_visibility,
}