local sk_scene = require('sk_scene')
local sk_defintion_writer = require('sk_definition_writer')
local room_export = require('tools.level_scripts.room_export')
local collision_export = require('tools.level_scripts.collision_export')
local sk_math = require('sk_math')

local room_doorways = {}

for i = 1,room_export.room_count do
    table.insert(room_doorways, {})
end

local doorways = {}

for doorway_index, doorway in pairs(sk_scene.nodes_for_type('@doorway')) do
    local quad = collision_export.create_collision_quad(
        doorway.node.meshes[1]:transform(doorway.node.full_transformation),
        0
    )

    local room_a = room_export.node_nearest_room_index(doorway.node, nil)
    local room_b, room_b_bb = room_export.node_nearest_room_index(doorway.node, room_a)

    if room_b_block then
        local room_b_center = room_b_bb:lerp(0.5)

        -- check if the doorway is facing room A
        if (room_b_center - quad.corner):dot(quad.plane.normal > 0) then
            room_a, room_b = room_b, room_a
        end
    end

    table.insert(room_doorways[room_a + 1], doorway_index - 1)
    table.insert(room_doorways[room_b + 1], doorway_index - 1)

    quad.corner = quad.corner + (-0.5 * quad.edgeA + -0.5 * quad.edgeB)
    quad.edgeALength = quad.edgeALength + 1.0
    quad.edgeBLength = quad.edgeBLength + 1.0

    table.insert(doorways, {
        quad,
        room_a,
        room_b,
        sk_defintion_writer.raw('DoorwayFlagsOpen')
    })
end

sk_defintion_writer.add_definition('doorways', 'struct Doorway[]', '_geo', doorways)

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

    sk_defintion_writer.add_definition('room_indices', 'short[]', '_geo', quad_indices)
    sk_defintion_writer.add_definition('room_cells', 'struct Rangeu16[]', '_geo', cell_contents)
    
    sk_defintion_writer.add_definition('room_doorways', 'short[]', '_geo', room_doorways[room_index])

    return {
        sk_defintion_writer.reference_to(quad_indices, 1),
        sk_defintion_writer.reference_to(cell_contents, 1),
        room_grid and room_grid.span_x or 0,
        room_grid and room_grid.span_z or 0,
        room_grid and room_grid.x or 0,
        room_grid and room_grid.z or 0,
        room_export.room_bb[room_index] or sk_math.box3(),
        sk_defintion_writer.reference_to(room_doorways[room_index], 1),
        #room_doorways[room_index],
    }
end

local rooms = {}

for i = 1,room_export.room_count do
    table.insert(rooms, generate_room(i))
end

sk_defintion_writer.add_definition('rooms', 'struct Room[]', '_geo', rooms)

local function find_coplanar_doorway(point)
    for index, doorway in pairs(doorways) do
        if collision_export.is_coplanar(doorway[1], point) then
            return index
        end
    end

    return 0
end

return {
    world = {
        rooms = sk_defintion_writer.reference_to(rooms, 1),
        doorways = sk_defintion_writer.reference_to(doorways, 1),
        roomCount = #rooms,
        doorwayCount = #doorways,
    },
    find_coplanar_doorway = find_coplanar_doorway,
}