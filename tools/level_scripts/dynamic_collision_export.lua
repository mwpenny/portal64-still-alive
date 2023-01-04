local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local animation = require('tools.level_scripts.animation')
local room_export = require('tools.level_scripts.room_export')

local dynamic_boxes = {}
local dynamic_boxes_original = {}

local function build_dynamic_box(box) 
    local parent_node_index = animation.get_bone_index_for_node(box.node)

    if not parent_node_index then
        return nil, nil
    end

    local parent_node = animation.get_bone_for_index(parent_node_index)

    if not parent_node then
        return nil, nil
    end

    local relative_transform = parent_node.full_transformation:inverse() * box.node.full_transformation

    local mesh_bb = box.node.meshes[1].bb

    local _, rotation = relative_transform:decompose()
    local pos = relative_transform * mesh_bb:lerp(0.5)

    return {
        {(mesh_bb.max - mesh_bb.min) * 0.5},
        pos,
        rotation,
        room_export.node_nearest_room_index(box.node),
        parent_node_index - 1,
    }, {
        node = box.node,
        parent_node_index = parent_node_index,
    }
end

for _, box in pairs(sk_scene.nodes_for_type('@dynamic_box')) do
    local dynamic_box, dynamic_box_original  = build_dynamic_box(box)

    if dynamic_box then
        table.insert(dynamic_boxes, dynamic_box)
        table.insert(dynamic_boxes_original, dynamic_box_original)
    end

end

sk_definition_writer.add_definition('dynamic_boxes', 'struct DynamicBoxDefinition[]', '_geo', dynamic_boxes)

return {
    dynamic_boxes = dynamic_boxes,
    dynamic_boxes_original = dynamic_boxes_original,
}