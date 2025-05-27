
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_mesh = require('sk_mesh')
local sk_math = require('sk_math')
local sk_input = require('sk_input')
local sk_transform = require('sk_transform')
local room_export = require('tools.level_scripts.room_export')
local animation = require('tools.level_scripts.animation')
local signals = require('tools.level_scripts.signals')

sk_definition_writer.add_header('"codegen/assets/materials/static.h"')
sk_definition_writer.add_header('"codegen/assets/strings/strings.h"')
sk_definition_writer.add_header('"levels/level_definition.h"')

local portalable_surfaces = {
    concrete_modular_wall001b = true,
    concrete_modular_wall001d = true,
    concrete_modular_ceiling001a = true,
    concrete_modular_floor001a = true,
    transparent_portal_surface = true,
}

local signal_elements = {}

local coplanar_tolerance = 0.01

local function is_coplanar(mesh, plane)
    for _, vertex in pairs(mesh.vertices) do
        if math.abs(plane:distance_to_point(vertex)) > coplanar_tolerance then
            return false
        end
    end

    return true
end

local function should_join_mesh(entry)
    return entry.should_join_mesh
end

local function bb_union_cost(a, b)
    local union = a:union(b)
    local intersection = a:intersection(b)

    return union:area() - a:area() - b:area() + intersection:area()
end

local function insert_or_merge(static_list, new_entry)
    if not should_join_mesh(new_entry) then
        table.insert(static_list, new_entry)
        return
    end 

    for _, other_entry in pairs(static_list) do
        if should_join_mesh(other_entry) and 
            other_entry.material_index == new_entry.material_index and
            other_entry.room_index == new_entry.room_index and 
            is_coplanar(new_entry.chunk.mesh, other_entry.plane) then
                
            other_entry.chunk.mesh = other_entry.chunk.mesh:join(new_entry.chunk.mesh)
            other_entry.original_bb = other_entry.chunk.mesh.bb
            return
        end
    end

    table.insert(static_list, new_entry)
end

local function bb_list(bb) 
    return {
        bb.min.x,
        bb.min.y,
        bb.min.z,

        bb.max.x,
        bb.max.y,
        bb.max.z,
    }
end

local function rotated_bb_list(untransformed_bb, transform, bb_scale)
    local _, rotation, _ = transform:decompose()
    local side_lengths = (untransformed_bb.max - untransformed_bb.min) * bb_scale

    return {
        (transform * untransformed_bb.min) * bb_scale,
        {
            rotation * sk_math.vector3(side_lengths.x),
            rotation * sk_math.vector3(0, side_lengths.y),
            rotation * sk_math.vector3(0, 0, side_lengths.z)
        }
    }
end

local function check_for_decals(portal_surface, all_static_ndes)
    if not portal_surface.accept_portals then
        return false
    end

    local box_with_padding = sk_math.box3(portal_surface.original_bb.min - 0.1, portal_surface.original_bb.max + 0.1)

    for _, other in pairs(all_static_ndes) do
        local material = other.chunk.mesh.material

        if material.renderMode and 
            material.renderMode.zMode == "ZMODE_DEC" and 
            portal_surface.transform_index == other.transform_index and
            is_coplanar(other.chunk.mesh, portal_surface.plane) and 
            box_with_padding:overlaps(other.original_bb) then
            return true
        end
    end
    
    return false
end

local function list_static_nodes(nodes)
    local result = {}
    local bb_scale = sk_input.settings.fixed_point_scale

    for k, v in pairs(nodes) do
        local renderChunks = sk_mesh.generate_render_chunks(v.node)

        local signal = sk_scene.find_named_argument(v.arguments, "indicator_lights")
        
        for _, chunkV in pairs(renderChunks) do
            local transform_index = animation.get_bone_index_for_node(v.node)

            local original_bb = chunkV.mesh.bb

            if transform_index then
                local bone = animation.get_bone_for_index(transform_index)
                local parent_pos = bone.full_transformation:decompose()
                chunkV.mesh = chunkV.mesh:transform(bone.full_transformation:inverse())
            
                original_bb = chunkV.mesh.bb
            end

            local plane = sk_math.plane3_with_point(chunkV.mesh.normals[1], chunkV.mesh.vertices[1])
            local should_join_mesh = true

            local portalable_material = chunkV.mesh.material and portalable_surfaces[chunkV.mesh.material.name]
            local no_portals = sk_scene.find_flag_argument(v.arguments, "no_portals")
            local accept_portals_override = sk_scene.find_flag_argument(v.arguments, "accept_portals")

            if no_portals and accept_portals_override then
                error("Static geometry '" .. table.concat(v.arguments, " ") .. "' cannot specify both 'no_portals' and 'accept_portals'")
            end

            local accept_portals = (portalable_material or accept_portals_override) and not no_portals
            local precise_culling = sk_scene.find_flag_argument(v.arguments, "precise_culling")

            if transform_index or signal or accept_portals or precise_culling or not is_coplanar(chunkV.mesh, plane) then
                should_join_mesh = false
            end
    
            insert_or_merge(result, {
                node = v.node, 
                chunk = chunkV,
                material_index = chunkV.material.macro_name,
                transform_index = transform_index,
                room_index = room_export.node_nearest_room_index(v.node) or 0,
                accept_portals = accept_portals,
                precise_culling = precise_culling,
                has_decals = false,
                signal = signal,
                original_bb = original_bb,
                plane = plane,
                should_join_mesh = should_join_mesh,
            })
        end
    end

    for _, node in pairs(result) do
        node.has_decals = check_for_decals(node, result)
    end

    return result;
end

local function debug_print_index(index, indent)
    indent = indent or ''

    if indent == '' then
        print('tree begin')
    end

    for key, node in pairs(index) do
        if type(node) ~= 'table' then
            print(indent .. key .. ' not table ' .. tostring(node))
        elseif node.children then
            print(indent .. key .. ' branch ' .. tostring(node.mesh_bb))
            debug_print_index(node.children, '    ' .. indent)
        else
            print(indent .. key .. ' leaf ' .. tostring(node.node.name))
        end
    end
end

local function serialize_static_index(index)
    local leaf_nodes = {}
    local branch_nodes = {}

    local function traverse_static_index(index, mesh_bb)
        local static_start = #leaf_nodes

        -- collect the leaf nodes first
        for _, child in pairs(index) do
            if not child.children then
                table.insert(leaf_nodes, child)
            end
        end

        local static_range = {min = static_start, max = #leaf_nodes}
        local total_descendant_count = 0

        local branch_node = {
            box = bb_list(mesh_bb),
            staticRange = static_range,
            siblingOffset = 0,
        }

        table.insert(branch_nodes, branch_node)

        -- recursively build the rest of the nodes
        for _, child in pairs(index) do
            if child.children then
                total_descendant_count = total_descendant_count + traverse_static_index(child.children, child.mesh_bb)
            end
        end

        branch_node.siblingOffset = total_descendant_count + 1

        return total_descendant_count + 1
    end

    local room_bb = index[1].mesh_bb

    for _, child in pairs(index) do
        room_bb = room_bb:union(child.mesh_bb)
    end

    traverse_static_index(index, room_bb)

    return leaf_nodes, branch_nodes
end

local axis_index_to_name = {'x', 'y', 'z'}

local function bb_center(bb, axis_name) 
    return (bb.min[axis_name] + bb.max[axis_name]) * 0.5
end

local function build_bvh_recursive(nodes) 
    if #nodes <= 1 then
        return nodes
    end

    local split_halfs = {}
    local split_score = #nodes + 1

    for _, axis_name in pairs(axis_index_to_name) do
        local center_min = bb_center(nodes[1].mesh_bb, axis_name)
        local center_max = center_min
    
        for _, node in pairs(nodes) do
            local node_center = bb_center(node.mesh_bb, axis_name)
    
            center_min = math.min(center_min, node_center)
            center_max = math.max(center_max, node_center)
        end
    
        local left = {}
        local right = {}
    
        local center = (center_min + center_max) * 0.5
    
        for _, node in pairs(nodes) do
            if bb_center(node.mesh_bb, axis_name) < center then
                table.insert(left, node)
            else
                table.insert(right, node)
            end
        end

        local score = math.abs(#left - #right)

        if score < split_score then
            split_score = score
            split_halfs = {left, right}
        end
    end

    local total_bb = nodes[1].mesh_bb
    
    for _, node in pairs(nodes) do
        total_bb = total_bb:union(node.mesh_bb)
    end

    if split_score == #nodes then
        return {{
            children = {table.unpack(split_halfs[1]), table.unpack(split_halfs[2])},
            mesh_bb = total_bb,
        }}
    end

    return {{
        children = {
            table.unpack(build_bvh_recursive(split_halfs[1])),
            table.unpack(build_bvh_recursive(split_halfs[2])),
        },
        mesh_bb = total_bb,
    }}
end

local MAX_STATIC_INDEX_DEPTH = 4

local function build_static_index(room_static_nodes)
    local animated_nodes = {}
    local non_moving_nodes = {}

    for _, node in pairs(room_static_nodes) do
        if node.transform_index then
            table.insert(animated_nodes, node)
        else
            table.insert(non_moving_nodes, node)
        end
    end

    local non_moving_nodes = build_bvh_recursive(non_moving_nodes)

    local static_result, branch_index = serialize_static_index(non_moving_nodes)

    local animated_min = #static_result

    for _, animated_node in pairs(animated_nodes) do
        table.insert(static_result, animated_node)
    end

    return {
        static_nodes = static_result,
        branch_index = branch_index,
        animated_range = {
            min = animated_min,
            max = #static_result,
        },
    }
end

local function process_static_nodes(nodes)
    local result = {}
    local bb_scale = sk_input.settings.fixed_point_scale

    -- For precise culling
    local static_bounding_boxes = {}

    local source_nodes = list_static_nodes(nodes)

    for _, source_node in pairs(source_nodes) do
        local gfxName = sk_mesh.generate_mesh({source_node.chunk}, "_geo", {defaultMaterial = source_node.chunk.material})

        local mesh_bb = source_node.original_bb * bb_scale

        mesh_bb.min.x = math.floor(mesh_bb.min.x + 0.5)
        mesh_bb.min.y = math.floor(mesh_bb.min.y + 0.5)
        mesh_bb.min.z = math.floor(mesh_bb.min.z + 0.5)

        mesh_bb.max.x = math.floor(mesh_bb.max.x + 0.5)
        mesh_bb.max.y = math.floor(mesh_bb.max.y + 0.5)
        mesh_bb.max.z = math.floor(mesh_bb.max.z + 0.5)

        -- Save the object's exact (i.e., rotated) bounding box to use for culling
        -- More expensive at runtime than BVH's AABB checks, but prevents geometry behind portals from showing
        if source_node.precise_culling then
            local transform = source_node.node.full_transformation
            local untransformed = source_node.chunk.mesh:transform(transform:inverse())
            local rotated_bb = rotated_bb_list(untransformed.bb, transform, bb_scale)

            source_node.bounding_box_index = #static_bounding_boxes
            table.insert(static_bounding_boxes, rotated_bb)
        end

        table.insert(result, {
            node = source_node.node, 
            mesh = source_node.chunk.mesh,
            mesh_bb = mesh_bb,
            display_list = sk_definition_writer.raw(gfxName), 
            material_index = sk_definition_writer.raw(source_node.chunk.material.macro_name),
            transform_index = source_node.transform_index,
            bounding_box_index = source_node.bounding_box_index,
            room_index = source_node.room_index,
            accept_portals = source_node.accept_portals,
            has_decals = source_node.has_decals,
            signal = source_node.signal,
        })
    end

    sk_definition_writer.add_definition('static_bounding_boxes', 'struct RotatedBox[]', '_geo', static_bounding_boxes);

    table.sort(result, function(a, b)
        return a.room_index < b.room_index
    end)

    local last_boundary = 1
    local room_bvh_list = {}

    local final_static_list = {}

    for room_index = 0,room_export.room_count-1 do
        local next_boundary = last_boundary

        while next_boundary <= #result and result[next_boundary].room_index == room_index do
            next_boundary = next_boundary + 1
        end

        local room_elements = {table.unpack(result, last_boundary, next_boundary - 1)}
        local room_bvh = build_static_index(room_elements)

        local animated_boxes = {}

        for i = room_bvh.animated_range.min+1, room_bvh.animated_range.max do
            table.insert(animated_boxes, bb_list(room_bvh.static_nodes[i].mesh_bb))
        end

        sk_definition_writer.add_definition('animated_boxes', 'struct BoundingBoxs16[]', '_geo', animated_boxes);

        room_bvh.animated_range.min = room_bvh.animated_range.min + #final_static_list
        room_bvh.animated_range.max = room_bvh.animated_range.max + #final_static_list

        for _, branch_node in pairs(room_bvh.branch_index) do
            branch_node.staticRange.min = branch_node.staticRange.min + #final_static_list
            branch_node.staticRange.max = branch_node.staticRange.max + #final_static_list
        end

        sk_definition_writer.add_definition('bvh', 'struct StaticContentBox[]', '_geo', room_bvh.branch_index);

        table.insert(room_bvh_list, {
            boxIndex = sk_definition_writer.reference_to(room_bvh.branch_index, 1),
            animatedBoxes = sk_definition_writer.reference_to(animated_boxes, 1),
            animatedRange = room_bvh.animated_range,
            boxCount = #room_bvh.branch_index,
        })

        for _, node in pairs(room_bvh.static_nodes) do
            table.insert(final_static_list, node)
        end

        last_boundary = next_boundary
    end

    sk_definition_writer.add_definition('room_bvh', 'struct StaticIndex[]', '_geo', room_bvh_list);

    return final_static_list, room_bvh_list, static_bounding_boxes;
end

local static_nodes, room_bvh_list, static_bounding_boxes = process_static_nodes(sk_scene.nodes_for_type('@static'))

local static_content_elements = {}

local room_ranges = {}

local bb_scale_inv = 1 / sk_input.settings.fixed_point_scale

for index, static_node in pairs(static_nodes) do
    if static_node.signal then
        local signal_number = signals.signal_number_for_name(static_node.signal)
        
        while #signal_elements < signal_number do
            table.insert(signal_elements, {})
        end

        table.insert(signal_elements[signal_number], #static_content_elements)
    end

    table.insert(static_content_elements, {
        displayList = static_node.display_list,
        center = static_node.mesh_bb:lerp(0.5) * bb_scale_inv,
        materialIndex = static_node.material_index,
        transformIndex = static_node.transform_index and (static_node.transform_index - 1) or sk_definition_writer.raw('NO_TRANSFORM_INDEX'),
        boundingBoxIndex = static_node.bounding_box_index or sk_definition_writer.raw('NO_BOUNDING_BOX_INDEX'),
    })

    good_index = index - 1

    while (#room_ranges <= static_node.room_index) do
        table.insert(room_ranges, {
            good_index,
            good_index
        })
    end

    local room_range = room_ranges[static_node.room_index + 1]

    room_range[2] = good_index + 1
end

sk_definition_writer.add_definition("static", "struct StaticContentElement[]", "_geo", static_content_elements)
sk_definition_writer.add_definition("room_mapping", "struct Rangeu16[]", "_geo", room_ranges)

local signal_indices = {}
local signal_ranges = {}

for _, element in pairs(signal_elements) do
    table.insert(signal_ranges, {#signal_indices, #signal_indices + #element})

    for _, index in pairs(element) do
        table.insert(signal_indices, index)
    end
end

sk_definition_writer.add_definition("signal_ranges", "struct Rangeu16[]", "_geo", signal_ranges)
sk_definition_writer.add_definition('signal_indices', 'u16[]', '_geo', signal_indices)

return {
    static_nodes = static_nodes,
    static_content_elements = static_content_elements,
    static_bounding_boxes = static_bounding_boxes,
    room_ranges = room_ranges,
    signal_ranges = signal_ranges,
    signal_indices = signal_indices,
    room_bvh_list = room_bvh_list,
}