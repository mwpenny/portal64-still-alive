local sk_scene = require('sk_scene')
local sk_animation = require('sk_animation')
local sk_scene = require('sk_scene')
local sk_definition_writer = require('sk_definition_writer')

local armature_bones_by_name = {}

for _, node in pairs(sk_scene.nodes_for_type('@anim')) do
    local name = node.arguments[1]
    local existing = armature_bones_by_name[name]

    if existing then
        table.insert(existing, node.node)
    else
        armature_bones_by_name[name] = {node.node}
    end
end

local armatures = {}

local node_to_bone_index = {}
local node_to_armature_index = {}
local bones_as_array = {}

for name, nodes in pairs(armature_bones_by_name) do
    table.insert(armatures, {name = name, armature = sk_animation.build_armature(nodes)})
end

table.sort(armatures, function(a, b)
    return a.name < b.name
end)

local animated_nodes = {}

for index, armature in pairs(armatures) do
    -- build an index used to attached parts of the scene
    -- to animation nodes later
    for _, node in pairs(armature.armature.nodes) do
        table.insert(bones_as_array, node)
        local bone_index = #bones_as_array
        
        sk_scene.for_each_node(node, function(child_node)
            node_to_bone_index[child_node] = bone_index
            node_to_armature_index[child_node] = index
        end)
    end

    local armature_data = sk_animation.build_armature_data(armature.armature, nil, armature.name, '_geo')
    local animation_clips = {}

    for _, animation in pairs(sk_animation.filter_animations_for_armature(armature.armature, sk_scene.scene.animations)) do
        local clip = sk_animation.build_animation_clip(animation, armature.armature, '_anim')
        sk_definition_writer.add_macro(armature.name .. '_' .. animation.name, tostring(#animation_clips))
        table.insert(animation_clips, clip)
    end

    sk_definition_writer.add_definition(armature.name .. '_clips', 'struct SKAnimationClip[]', '_geo', animation_clips)

    table.insert(animated_nodes, {
        armature = armature_data,
        clips = sk_definition_writer.reference_to(animation_clips, 1),
        clipCount = #animation_clips
    })
end

local function get_bone_index_for_node(node)
    return node_to_bone_index[node]
end

local function get_bone_for_index(index)
    return bones_as_array[index]
end

sk_definition_writer.add_definition('anim', 'struct AnimationInfo[]', '_geo', animated_nodes)

return {
    animated_nodes = animated_nodes,
    get_bone_index_for_node = get_bone_index_for_node,
    get_bone_for_index = get_bone_for_index,
}