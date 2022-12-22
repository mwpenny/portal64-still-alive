
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_mesh = require('sk_mesh')
local room_export = require('tools.level_scripts.room_export')

sk_definition_writer.add_header('"../build/assets/materials/static.h"')
sk_definition_writer.add_header('"levels/level_definition.h"')

local function proccessStaticNodes(nodes)
    local result = {}

    for k, v in pairs(nodes) do
        local renderChunks = sk_mesh.generate_render_chunks(v.node)
        
        for _, chunkV in pairs(renderChunks) do
            local gfxName = sk_mesh.generate_mesh({chunkV}, "_geo", {defaultMaterial = chunkV.material})
    
            table.insert(result, {
                node = v.node, 
                mesh = chunkV.mesh,
                display_list = sk_definition_writer.raw(gfxName), 
                material_index = sk_definition_writer.raw(chunkV.material.macro_name)
            })
        end
    end

    return result;
end

local static_nodes = proccessStaticNodes(sk_scene.nodes_for_type('@static'))

for _, static_node in pairs(static_nodes) do
    static_node.room_index = room_export.node_nearest_room_index(static_node.node) or 0
end

local static_content_elements = {}

table.sort(static_nodes, function(a, b)
    return a.room_index < b.room_index
end)

local room_ranges = {}

for index, static_node in pairs(static_nodes) do
    table.insert(static_content_elements, {
        displayList = static_node.display_list,
        materialIndex = static_node.material_index
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

return {
    static_nodes = static_nodes,
    static_content_elements = static_content_elements,
    room_ranges = room_ranges,
}