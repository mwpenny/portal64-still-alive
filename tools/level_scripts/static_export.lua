
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_mesh = require('sk_mesh')
local sk_input = require('sk_input')
local room_export = require('tools.level_scripts.room_export')
local animation = require('tools.level_scripts.animation')

sk_definition_writer.add_header('"../build/assets/materials/static.h"')
sk_definition_writer.add_header('"levels/level_definition.h"')

local portalable_surfaces = {
    concrete_modular_wall001d = true,
    concrete_modular_ceiling001a = true,
    concrete_modular_floor001a = true,
}

local function proccessStaticNodes(nodes)
    local result = {}
    local bb_scale = sk_input.settings.fixed_point_scale

    for k, v in pairs(nodes) do
        local renderChunks = sk_mesh.generate_render_chunks(v.node)
        
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

            local gfxName = sk_mesh.generate_mesh({chunkV}, "_geo", {defaultMaterial = chunkV.material})

            local mesh_bb = original_bb * bb_scale

            mesh_bb.min.x = math.floor(mesh_bb.min.x + 0.5)
            mesh_bb.min.y = math.floor(mesh_bb.min.y + 0.5)
            mesh_bb.min.z = math.floor(mesh_bb.min.z + 0.5)

            mesh_bb.max.x = math.floor(mesh_bb.max.x + 0.5)
            mesh_bb.max.y = math.floor(mesh_bb.max.y + 0.5)
            mesh_bb.max.z = math.floor(mesh_bb.max.z + 0.5)
    
            table.insert(result, {
                node = v.node, 
                mesh = chunkV.mesh,
                mesh_bb = mesh_bb,
                display_list = sk_definition_writer.raw(gfxName), 
                material_index = sk_definition_writer.raw(chunkV.material.macro_name),
                transform_index = transform_index,
                room_index = room_export.node_nearest_room_index(v.node) or 0,
                accept_portals = chunkV.mesh.material and portalable_surfaces[chunkV.mesh.material.name] and not sk_scene.find_flag_argument(v.arguments, "no_portals"),
            })
        end
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

return {
    static_nodes = static_nodes,
    static_content_elements = static_content_elements,
    static_bounding_boxes = static_bounding_boxes,
    room_ranges = room_ranges,
}