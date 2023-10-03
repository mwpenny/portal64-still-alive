
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_mesh = require('sk_mesh')
local sk_math = require('sk_math')
local sk_input = require('sk_input')
local room_export = require('tools.level_scripts.room_export')
local animation = require('tools.level_scripts.animation')
local signals = require('tools.level_scripts.signals')

sk_definition_writer.add_header('"../build/assets/materials/static.h"')
sk_definition_writer.add_header('"levels/level_definition.h"')

local portalable_surfaces = {
    concrete_modular_wall001d = true,
    concrete_modular_ceiling001a = true,
    concrete_modular_floor001a = true,
}

local signal_elements = {}

local coplanar_tolerance = 0.1

local function is_coplanar(mesh, plane)
    for _, vertex in pairs(mesh.vertices) do
        if math.abs(plane:distance_to_point(vertex)) > coplanar_tolerance then
            return false
        end
    end

    return true
end

local function should_join_mesh(entry)
    return entry.plane ~= nil
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
            
                original_bb.min = original_bb.min - parent_pos
                original_bb.max = original_bb.max - parent_pos
            end

            local plane = sk_math.plane3_with_point(chunkV.mesh.normals[1], chunkV.mesh.vertices[1])
            local accept_portals =chunkV.mesh.material and portalable_surfaces[chunkV.mesh.material.name] and not sk_scene.find_flag_argument(v.arguments, "no_portals")

            if transform_index or signal or accept_portals or not is_coplanar(chunkV.mesh, plane) then
                plane = nil
            end
    
            insert_or_merge(result, {
                node = v.node, 
                chunk = chunkV,
                material_index = chunkV.material.macro_name,
                transform_index = transform_index,
                room_index = room_export.node_nearest_room_index(v.node) or 0,
                accept_portals = accept_portals,
                signal = signal,
                original_bb = original_bb,
                plane = plane,
            })
        end
    end

    return result;
end

local function proccessStaticNodes(nodes)
    local result = {}
    local bb_scale = sk_input.settings.fixed_point_scale

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

        table.insert(result, {
            node = source_node.node, 
            mesh = source_node.chunk.mesh,
            mesh_bb = mesh_bb,
            display_list = sk_definition_writer.raw(gfxName), 
            material_index = sk_definition_writer.raw(source_node.chunk.material.macro_name),
            transform_index = source_node.transform_index,
            room_index = source_node.room_index,
            accept_portals = source_node.accept_portals,
            signal = source_node.signal,
        })
    end

    table.sort(result, function(a, b)
        return a.room_index < b.room_index
    end)

    return result;
end

local static_nodes = proccessStaticNodes(sk_scene.nodes_for_type('@static'))

local static_content_elements = {}

local room_ranges = {}
local static_bounding_boxes = {}

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
        materialIndex = static_node.material_index,
        transformIndex = static_node.transform_index and (static_node.transform_index - 1) or sk_definition_writer.raw('NO_TRANSFORM_INDEX'),
    })
    table.insert(static_bounding_boxes, {
        static_node.mesh_bb.min.x,
        static_node.mesh_bb.min.y,
        static_node.mesh_bb.min.z,

        static_node.mesh_bb.max.x,
        static_node.mesh_bb.max.y,
        static_node.mesh_bb.max.z,
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
sk_definition_writer.add_definition('bounding_boxes', 'struct BoundingBoxs16[]', '_geo', static_bounding_boxes)

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
}