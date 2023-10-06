local sk_scene = require('sk_scene')
local sk_animation = require('sk_animation')
local sk_scene = require('sk_scene')
local sk_definition_writer = require('sk_definition_writer')

local armature_bones_by_name = {}

for _, node in pairs(sk_scene.nodes_for_type('@anim')) do
    local name = node.arguments[1]
    local existing = armature_bones_by_name[name]

    if existing then
        table.insert(existing.nodes, node.node)
    else
        armature_bones_by_name[name] = {nodes = {node.node}, arguments = node.arguments}
    end
end

local armatures = {}

local armature_indices_by_name = {}
local animation_indices_by_name = {}

local node_to_bone_index = {}
local node_to_armature_index = {}
local bones_as_array = {}

for name, data in pairs(armature_bones_by_name) do
    table.insert(armatures, {name = name, armature = sk_animation.build_armature(data.nodes), sound_type = sk_scene.find_named_argument(data.arguments, "sound_type")})
end

table.sort(armatures, function(a, b)
    return a.name < b.name
end)

local function parse_animation_name(name)
    local _, str_end = string.find(name, '|')

    if str_end then
        return string.sub(name, str_end + 1)
    end

    return name
end

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

    local animation_names = {}

    for animation_index, animation in pairs(sk_animation.filter_animations_for_armature(armature.armature, sk_scene.scene.animations)) do
        local clip = sk_animation.build_animation_clip(animation, armature.armature, '_anim')
        sk_definition_writer.add_macro(armature.name .. '_' .. animation.name, tostring(#animation_clips))
        table.insert(animation_clips, clip)

        animation_names[parse_animation_name(animation.name)] = sk_definition_writer.raw(sk_definition_writer.add_macro(armature.name .. '_ANIMATION_' .. animation.name, animation_index - 1))
    end

    sk_definition_writer.add_definition(armature.name .. '_clips', 'struct SKAnimationClip[]', '_geo', animation_clips)

    table.insert(animated_nodes, {
        armature = armature_data,
        clips = sk_definition_writer.reference_to(animation_clips, 1),
        clipCount = #animation_clips,
        soundType = sk_definition_writer.raw('AnimationSoundType' .. (armature.sound_type or 'None')),
    })

    armature_indices_by_name[armature.name] = sk_definition_writer.raw(sk_definition_writer.add_macro('ARMATURE_' .. armature.name, tostring(index - 1)))
    animation_indices_by_name[armature.name] = animation_names
end

local function get_bone_index_for_node(node)
    return node_to_bone_index[node]
end

local function get_bone_for_index(index)
    return bones_as_array[index]
end

local function get_armature_index_with_name(name)
    return armature_indices_by_name[name]
end

local function get_animation_with_name(armature_name, animation_name)
    local armature = animation_indices_by_name[armature_name]
    
    if not armature then
        return nil
    end

    return armature[animation_name]
end

sk_definition_writer.add_definition('anim', 'struct AnimationInfo[]', '_geo', animated_nodes)

return {
    animated_nodes = animated_nodes,
    get_bone_index_for_node = get_bone_index_for_node,
    get_bone_for_index = get_bone_for_index,
    get_armature_index_with_name = get_armature_index_with_name,
    get_animation_with_name = get_animation_with_name,
}