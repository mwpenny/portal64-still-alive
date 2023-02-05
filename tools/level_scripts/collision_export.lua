
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_mesh = require('sk_mesh')
local sk_math = require('sk_math')
local room_export = require('tools.level_scripts.room_export')

local COLLISION_GRID_CELL_SIZE = 4

local function build_collision_grid(boundaries)
    local x = math.floor(boundaries.min.x)
    local z = math.floor(boundaries.min.z)

    local span_x = math.ceil((boundaries.max.x - x) / COLLISION_GRID_CELL_SIZE)
    local span_z = math.ceil((boundaries.max.z - z) / COLLISION_GRID_CELL_SIZE)

    local cells = {}

    for i = 1,span_x do
        local cell_row = {}

        for j = 1,span_z do
            table.insert(cell_row, {})
        end

        table.insert(cells, cell_row)
    end

    return {
        x = x,
        z = z,
        span_x = span_x,
        span_z = span_z,
        cells = cells,
    }
end

local function add_to_collision_grid(grid, box, value)
    local min_x = math.floor((box.min.x - grid.x) / COLLISION_GRID_CELL_SIZE)
    local max_x = math.floor((box.max.x - grid.x) / COLLISION_GRID_CELL_SIZE)
    local min_z = math.floor((box.min.z - grid.z) / COLLISION_GRID_CELL_SIZE)
    local max_z = math.floor((box.max.z - grid.z) / COLLISION_GRID_CELL_SIZE)

    if (max_x < 0) then max_x = 0 end
    if (min_x >= grid.span_x) then min_x = grid.span_x - 1 end

    if (max_z < 0) then max_z = 0 end
    if (min_z >= grid.span_z) then min_z = grid.span_z - 1 end

    for curr_x = min_x,max_x do
        for curr_z = min_z,max_z do
            if (curr_x >= 0 and curr_x < grid.span_x and curr_z >= 0 and curr_z < grid.span_z) then
                table.insert(grid.cells[curr_x + 1][curr_z + 1], value)
            end
        end
    end
end

local function parse_quad_thickness(node_info)
    local thickness = sk_scene.find_named_argument(node_info.arguments, "thickness")

    if (thickness) then
        return tonumber(thickness)
    end

    return 0
end

local collider_nodes = sk_scene.nodes_for_type("@collision")

for _, node in pairs(collider_nodes) do
    node.room_index = room_export.node_nearest_room_index(node.node)
end

table.sort(collider_nodes, function (a, b)
    return a.room_index < b.room_index
end)

local SAME_TOLERANCE = 0.00001

local function bottom_right_compare(a, b)
    if (math.abs(a.x - b.x) > SAME_TOLERANCE) then
        return a.x < b.x
    end

    if (math.abs(a.y - b.y) > SAME_TOLERANCE) then
        return a.y < b.y
    end

    return a.z < b.z
end

local function find_min(array, predicate)
    local result = array[1]
    local result_index = 1

    for index, current in pairs(array) do
        if (predicate(current, result)) then
            result = current
            result_index = index
        end
    end

    return result, result_index
end

local function bottom_right_most_index(vertices)
    return find_min(vertices, bottom_right_compare)
end

local function find_most_opposite_edge(from_edge, edges)
    return find_min(edges, function(a, b)
        return a:dot(from_edge) < b:dot(from_edge)
    end)
end

local function find_adjacent_vertices(mesh, corner_index)
    local result = {}

    for _, face in pairs(mesh.faces) do
        for index_index, index in pairs(face) do
            if (index == corner_index) then
                local next_index = index_index + 1

                if (next_index > #face) then
                    next_index = 1
                end

                local prev_index = index_index - 1

                if (prev_index == 0) then
                    prev_index = #face
                end

                result[face[next_index]] = true
                result[face[prev_index]] = true
            end
        end
    end

    return result
end

local function create_collision_quad(mesh, thickness)
    local bottom_right_most = mesh.vertices[1]

    local corner_point, corner_index = bottom_right_most_index(mesh.vertices)

    local adjacent_indices = find_adjacent_vertices(mesh, corner_index)

    local edges_from_corner = {}

    for index, _ in pairs(adjacent_indices) do
        table.insert(edges_from_corner, mesh.vertices[index] - corner_point)
    end

    local edge_a_point = find_most_opposite_edge(edges_from_corner[1], edges_from_corner)
    local edge_b_point = find_most_opposite_edge(edge_a_point, edges_from_corner)

    local normal_sum = sk_math.vector3(0, 0, 0)

    for _, normal in pairs(mesh.normals) do
        normal_sum = normal_sum + normal
    end

    local final_normal = normal_sum:normalized()

    -- make sure the basis is right handed
    if edge_a_point:cross(edge_b_point):dot(final_normal) < 0 then
        edge_a_point, edge_b_point = edge_b_point, edge_a_point
    end

    local edge_a_normalized = edge_a_point:normalized()
    local edge_b_normalized = edge_b_point:normalized()


    return {
        corner = corner_point,
        edgeA = edge_a_normalized,
        edgeALength = edge_a_point:dot(edge_a_normalized),
        edgeB = edge_b_normalized,
        edgeBLength = edge_b_point:dot(edge_b_normalized),
        plane = {
            normal = final_normal,
            d = -corner_point:dot(final_normal),
        },
        thickness = thickness,
    }
end

local function collision_quad_bb(collision_quad)
    local min = collision_quad.corner
    local max = collision_quad.corner

    for x = 1,2 do
        for y = 1,2 do
            for z = 1,2 do
                local point = collision_quad.corner

                if (x == 2) then
                    point = point + collision_quad.edgeA * collision_quad.edgeALength
                end

                if (y == 2) then
                    point = point + collision_quad.edgeB * collision_quad.edgeBLength
                end

                if (z == 2) then
                    point = point - collision_quad.plane.normal * collision_quad.thickness
                end

                min = min:min(point)
                max = max:max(point)
            end
        end
    end

    return sk_math.box3(min, max)
end

local INSIDE_NORMAL_TOLERANCE = 0.1

local function is_coplanar(collision_quad, mesh)
    if sk_math.isVector3(mesh) then
        local offset = mesh - collision_quad.corner

        local z = offset:dot(collision_quad.plane.normal)

        if math.abs(z) >= INSIDE_NORMAL_TOLERANCE then
            return false
        end

        local x = offset:dot(collision_quad.edgeA)

        if x < -INSIDE_NORMAL_TOLERANCE or x > collision_quad.edgeALength + INSIDE_NORMAL_TOLERANCE then
            return false
        end

        local y = offset:dot(collision_quad.edgeB)

        if y < -INSIDE_NORMAL_TOLERANCE or y > collision_quad.edgeBLength + INSIDE_NORMAL_TOLERANCE then
            return false
        end

        return true
    end

    for _, vertex in pairs(mesh.vertices) do
        local offset = vertex - collision_quad.corner

        local z = offset:dot(collision_quad.plane.normal)

        if math.abs(z) >= INSIDE_NORMAL_TOLERANCE then
            return false
        end
    end

    return true
end 

local colliders = {}
local collider_types = {}
local collision_objects = {}
local quad_rooms = {}

local room_bb = {}

local room_grids = {}

for index, node in pairs(collider_nodes) do
    local is_transparent = sk_scene.find_flag_argument(node.arguments, "transparent")

    local collision_layers = {}

    for _, arg in pairs(node.arguments) do
        if string.sub(arg, 1, #"CL_") == "CL_" then
            table.insert(collision_layers, 'COLLISION_LAYERS_' .. string.sub(arg, #"CL_" + 1))
        end
    end

    if #collision_layers == 0 then
        table.insert(collision_layers, 'COLLISION_LAYERS_STATIC')
        table.insert(collision_layers, 'COLLISION_LAYERS_BLOCK_BALL')
        table.insert(collision_layers, 'COLLISION_LAYERS_TANGIBLE')

        if is_transparent then 
            table.insert(collision_layers, 'COLLISION_LAYERS_TRANSPARENT')
        end
    end
        
    for _, mesh in pairs(node.node.meshes) do
        local global_mesh = mesh:transform(node.node.full_transformation)

        local collider = create_collision_quad(global_mesh, parse_quad_thickness(node))

        local named_entry = sk_scene.find_named_argument(node.arguments, "name")

        if (named_entry) then
            sk_definition_writer.add_macro(named_entry .. "_COLLISION_INDEX", #colliders)
        end

        local bb = collision_quad_bb(collider)
        
        if room_bb[node.room_index + 1] then
            room_bb[node.room_index + 1] = room_bb[node.room_index + 1]:union(bb)
        else
            room_bb[node.room_index + 1] = bb
        end

        table.insert(colliders, collider)
        table.insert(quad_rooms, node.room_index)

        local collider_type = {
            sk_definition_writer.raw("CollisionShapeTypeQuad"),
            sk_definition_writer.reference_to(collider),
            0,
            1,
            sk_definition_writer.null_value,
        }
        
        table.insert(collider_types, collider_type)

        table.insert(collision_objects, {
            sk_definition_writer.reference_to(collider_type),
            sk_definition_writer.null_value,
            bb,
            sk_definition_writer.raw(table.concat(collision_layers, ' | '))
        })
    end
end

for i = 1,room_export.room_count do
    if room_bb[i] then
        room_grids[i] = build_collision_grid(room_bb[i])
    end
end

for index, quad in pairs(colliders) do
    local room_grid = room_grids[quad_rooms[index] + 1]

    if room_grid then
        add_to_collision_grid(room_grid, collision_quad_bb(quad), index - 1)
    end
end

sk_definition_writer.add_definition("quad_colliders", "struct CollisionQuad[]", "_geo", colliders)
sk_definition_writer.add_definition("collider_types", "struct ColliderTypeData[]", "_geo", collider_types)
sk_definition_writer.add_definition("collision_objects", "struct CollisionObject[]", "_geo", collision_objects)

return {
    is_coplanar = is_coplanar,
    colliders = colliders,
    collision_quad_bb = collision_quad_bb,
    collision_objects = collision_objects,
    create_collision_quad = create_collision_quad,
    room_grids = room_grids,
}