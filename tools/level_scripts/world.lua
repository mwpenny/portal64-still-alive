local sk_scene = require('sk_scene')
local sk_definition_writer = require('sk_definition_writer')
local room_export = require('tools.level_scripts.room_export')
local collision_export = require('tools.level_scripts.collision_export')
local signals = require('tools.level_scripts.signals')
local sk_math = require('sk_math')

local room_doorways = {}

for i = 1,room_export.room_count do
    table.insert(room_doorways, {})
end

local doorways = {}

for doorway_index, doorway in pairs(sk_scene.nodes_for_type('@doorway')) do
    local quad = collision_export.collision_quad_from_mesh(
        doorway.node.meshes[1]:transform(doorway.node.full_transformation),
        0
    )

    local room_a = room_export.node_nearest_room_index(doorway.node, nil)
    local room_b, room_b_bb = room_export.node_nearest_room_index(doorway.node, room_a)

    local room_b_center = room_b_bb:lerp(0.5)

    -- Check if the doorway is facing room A
    if (room_b_center - quad.corner):dot(quad.plane.normal) > 0 then
        room_a, room_b = room_b, room_a
    end

    table.insert(room_doorways[room_a + 1], doorway_index - 1)
    table.insert(room_doorways[room_b + 1], doorway_index - 1)

    quad.corner = quad.corner + (-0.5 * quad.edgeA + -0.5 * quad.edgeB)
    quad.edgeALength = quad.edgeALength + 1.0
    quad.edgeBLength = quad.edgeBLength + 1.0

    for _, doorway in pairs(doorways) do
        if (doorway[2] == room_a and doorway[3] == room_b) or
           (doorway[2] == room_b and doorway[3] == room_a) then
            error('At most one doorway can connect two rooms. Found multiple doorways for rooms ' .. room_a .. ' and ' .. room_b .. '.')
        end
    end

    table.insert(doorways, {
        quad,
        room_a,
        room_b,
        sk_definition_writer.raw('DoorwayFlagsOpen')
    })
end

sk_definition_writer.add_definition('doorways', 'struct Doorway[]', '_geo', doorways)

local function find_coplanar_doorway(point)
    for index, doorway in pairs(doorways) do
        if collision_export.is_coplanar(doorway[1], point) then
            return index
        end
    end

    return 0
end

local doors = {}

local function parse_door_type(name)
    if name == '02' then
        return sk_definition_writer.raw('DoorType02')
    end

    return sk_definition_writer.raw('DoorType01')
end

for _, door in pairs(sk_scene.nodes_for_type('@door')) do
    local position, rotation = door.node.full_transformation:decompose()

    table.insert(doors, {
        location = position,
        rotation = rotation,
        doorwayIndex = find_coplanar_doorway(position) - 1,
        signalIndex = signals.signal_index_for_name(door.arguments[1]),
        doorType = parse_door_type(door.arguments[2])
    })
end

sk_definition_writer.add_definition('doors', 'struct DoorDefinition[]', '_geo', doors)

local function generate_room(room_index)
    local quad_indices = {}
    local cell_contents = {}

    local room_grid = collision_export.room_grids[room_index]

    if room_grid then
        local start_index = 0

        for _, z_range in pairs(room_grid.cells) do
            for _, indices in pairs(z_range) do
                for _, index in pairs(indices) do
                    table.insert(quad_indices, index)
                end

                local end_index = start_index + #indices
                table.insert(cell_contents, {
                    start_index,
                    end_index
                })

                start_index = end_index
            end
        end
    end

    sk_definition_writer.add_definition('room_indices', 'short[]', '_geo', quad_indices)
    sk_definition_writer.add_definition('room_cells', 'struct Rangeu16[]', '_geo', cell_contents)
    
    sk_definition_writer.add_definition('room_doorways', 'short[]', '_geo', room_doorways[room_index])

    return {
        sk_definition_writer.reference_to(quad_indices, 1),
        sk_definition_writer.reference_to(cell_contents, 1),
        room_grid and room_grid.span_x or 0,
        room_grid and room_grid.span_z or 0,
        room_grid and room_grid.x or 0,
        room_grid and room_grid.z or 0,
        room_export.room_bb[room_index] or sk_math.box3(),
        sk_definition_writer.reference_to(room_doorways[room_index], 1),
        #room_doorways[room_index],
        room_export.room_non_visibility[room_index]
    }
end

local rooms = {}

for i = 1,room_export.room_count do
    table.insert(rooms, generate_room(i))
end

sk_definition_writer.add_definition('rooms', 'struct Room[]', '_geo', rooms)

return {
    world = {
        rooms = sk_definition_writer.reference_to(rooms, 1),
        doorways = sk_definition_writer.reference_to(doorways, 1),
        roomCount = #rooms,
        doorwayCount = #doorways,
    },
    doors = doors,
    find_coplanar_doorway = find_coplanar_doorway,
}