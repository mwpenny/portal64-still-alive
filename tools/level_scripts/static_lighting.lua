
local sk_scene = require('sk_scene')
local sk_math = require('sk_math');

local ambient_nodes = sk_scene.nodes_for_type('@ambient')

local ambient_blocks = {}

local function build_ambient_block(ambient_mesh)
    local midpoint = sk_math.vector3(0, 0, 0)
    local min = ambient_mesh.vertices[1]
    local max = ambient_mesh.vertices[1]

    for _, vector in pairs(ambient_mesh.vertices) do
        midpoint = midpoint + vector
        min = min:min(vector)
        max = max:max(vector)
    end

    midpoint = midpoint * (1 / #ambient_mesh.vertices)

    local colors = {}

    for index, vector in pairs(ambient_mesh.vertices) do
        local corner_index = 1

        if (vector.x > midpoint.x) then
            corner_index = corner_index + 1
        end

        if (vector.y > midpoint.y) then
            corner_index = corner_index + 2
        end

        if (vector.z > midpoint.z) then
            corner_index = corner_index + 4
        end

        colors[corner_index] = ambient_mesh.colors[1] and ambient_mesh.colors[1][index]
    end

    return {
        bb = sk_math.box3(min, max),
        colors = colors,
    }
end

local function evaluate_ambient_block(block, pos, normal)
    local lerp_values = block.bb:unlerp(pos):min(sk_math.vector3(1, 1, 1)):max(sk_math.vector3(0, 0, 0))

    local x0 = block.colors[1]:lerp(block.colors[2], lerp_values.x)
    local x1 = block.colors[3]:lerp(block.colors[4], lerp_values.x)
    local x2 = block.colors[5]:lerp(block.colors[6], lerp_values.x)
    local x3 = block.colors[7]:lerp(block.colors[8], lerp_values.x)

    local y0 = x0:lerp(x1, lerp_values.y)
    local y1 = x2:lerp(x3, lerp_values.y)

    return y0:lerp(y1, lerp_values.z)
end

for index, ambient_node in pairs(ambient_nodes) do
    for _, mesh in pairs(ambient_node.node.meshes) do
        local global_mesh = mesh:transform(ambient_node.node.full_transformation)

        table.insert(ambient_blocks, build_ambient_block(global_mesh))
    end
end

local function find_nearest_ambient_boxes(pos, max_count)
    local boxes = {};
    local distances = {};

    for _, ambient_block in pairs(ambient_blocks) do
        local distance = ambient_block.bb:distance_to_point(pos)

        -- if inside a block just return it
        if distance <= 0 then
            return {ambient_block}, {1}
        end

        local insert_index = #boxes + 1

        for index, other_distance in pairs(distances) do
            if distance < other_distance then
                insert_index = index
            end
        end

        if insert_index < max_count then
            table.insert(boxes, insert_index, ambient_block)
            table.insert(distances, insert_index, distance)

            if #boxes > max_count then
                table.remove(boxes)
                table.remove(distances)
            end
        end
    end

    if #distances == 1 then
        return boxes, {1}
    end

    local weights = {}
    local total_weight = 0

    for index, distance in pairs(distances) do
        total_weight = total_weight + distance
        weights[index] = distance
    end

    for index, _ in pairs(weights) do
        weights[index] = 1 - weights[index] / total_weight
    end

    return boxes, weights
end

local function light_vertex(pos, normal)
    local boxes, weights = find_nearest_ambient_boxes(pos, 2)

    if #boxes == 0 then
        return sk_math.color4(1, 1, 1, 1)
    end

    local result = sk_math.color4(0, 0, 0, 0)
    
    for index, block in pairs(boxes) do
        result = result + evaluate_ambient_block(block, pos, normal) * weights[index]
    end

    return result
end

return {
    light_vertex = light_vertex,
}