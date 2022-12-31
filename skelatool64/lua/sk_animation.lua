--- @module sk_animation

local sk_scene = require('sk_scene')
local sk_input = require('sk_input')
local sk_math = require('sk_math')
local sk_transform = require('sk_transform')
local sk_definition_writer = require('sk_definition_writer')

local node_order = {}
local current_node_index = 1

sk_scene.for_each_node(sk_scene.scene.root, function(node)
    node_order[node] = current_node_index
    current_node_index = current_node_index + 1
end)

local function find_interpolated_point(key_list, time)
    for index, key in pairs(key_list) do
        if key.time >= time then
            if index == 1 then
                return key.value, key.value, 0
            else
                local prev_key = key_list[index - 1]

                local delta_time = key.time - prev_key.time

                if delta_time == 0.0 then
                    return key.value, key.value, 0
                else
                    return prev_key.value, key.value, (time - prev_key.time) / delta_time
                end
            end
        end
    end

    if #key_list > 0 then
        return key_list[#key_list].value, key_list[#key_list].value, 1
    end

    return nil, nil, 0
end

local function evaluate_channel_at(channel, time)
    local prev_pos, next_pos, pos_lerp = find_interpolated_point(channel.position_keys, time)
    local prev_rot, next_rot, rot_lerp = find_interpolated_point(channel.rotation_keys, time)
    local prev_scale, next_scale, scale_lerp = find_interpolated_point(channel.scaling_keys, time)

    return (prev_pos and prev_pos:lerp(next_pos, pos_lerp) or sk_math.vector3(0, 0, 0)), 
        (prev_rot and prev_rot:slerp(next_rot, rot_lerp) or sk_math.quaternion(0, 0, 0, 1)),
        (prev_scale and prev_scale:lerp(next_scale, scale_lerp) or sk_math.vector3(1, 1, 1))
end

local function evaluate_animation_at(node_pose, animation, time)
    for _, channel in pairs(animation.channels) do
        local node = sk_scene.node_with_name(channel.node_name)

        if node then 
            local pos, rot, scale = evaluate_channel_at(channel, time)

            local local_transform = sk_transform.from_pos_rot_scale(pos, rot, scale)

            if node.parent then
                node_pose[node] = local_transform
            else
                node_pose[node] = sk_input.settings.fixed_point_transform * local_transform
            end
        end
    end
end

local Armature = {}

--- @function build_armature
--- @tparam {sk_scene.Node,...} animation_nodes
--- @treturn Armature
local function build_armature(animation_nodes)
    local nodes = {table.unpack(animation_nodes)}

    table.sort(nodes, function(a, b)
        return (node_order[a] or 0) < (node_order[b] or 0)
    end)

    local node_to_index = {}

    for index, node in pairs(nodes) do
        node_to_index[node] = index
    end

    return setmetatable({
        nodes = nodes,
        node_to_index = node_to_index,
    }, Armature)
end

local function add_to_node_pose(node_pose, node)
    if node_pose[node] then
        return
    end
    
    if node.parent then
        node_pose[node] = node.transformation

        add_to_node_pose(node_pose, node.parent)
    else
        node_pose[node] = node.full_transformation
    end
end

local function build_node_pose(armature, node, node_pose)
    local result = nil

    while node do 
        result = result and (node_pose[node] * result) or node_pose[node]
        node = node.parent

        -- only build the transform relative to the ancestor in the chain
        if node and armature:has_node(node) then
            return result
        end
    end

    return result
end

local function build_quat_pose(quat)
    if quat.w < 0 then
        return {
            math.floor((-quat.x * 32767) + 0.5),
            math.floor((-quat.y * 32767) + 0.5),
            math.floor((-quat.z * 32767) + 0.5)
        }
    else
        return {
            math.floor((quat.x * 32767) + 0.5),
            math.floor((quat.y * 32767) + 0.5),
            math.floor((quat.z * 32767) + 0.5)
        }
    end
end

local function build_armature_pose(armature, node_pose, result)
    for _, node in pairs(armature.nodes) do
        local pose = build_node_pose(armature, node, node_pose)

        local pos, rot = pose:decompose()

        pos = pos * sk_input.settings.fixed_point_scale

        table.insert(result, {
            sk_math.vector3(math.floor(pos.x + 0.5), math.floor(pos.y + 0.5), math.floor(pos.z + 0.5)),
            build_quat_pose(rot)
        })
    end
end

local function build_animation(armature, animation)
    local n_frames = math.ceil(animation.duration * sk_input.settings.ticks_per_second / animation.ticks_per_second)

    local node_pose = {}

    for _, node in pairs(armature.nodes) do
        add_to_node_pose(node_pose, node)
    end

    local frames = {}

    for frame_index = 1,n_frames do
        local time = (frame_index - 1) * animation.ticks_per_second / sk_input.settings.ticks_per_second
        -- populate node_pose from animation
        evaluate_animation_at(node_pose, animation, time)
        -- generate frame for armature
        build_armature_pose(armature, node_pose, frames)
    end

    return frames, n_frames
end

--- @table Clip
--- @tfield number nFrames
--- @tfield number nBones
--- @tfield sk_definition_writer.RefType frames
--- @tfield number fps

--- @function build_animation_clip
--- @tparam sk_scene.Animation animation
--- @tparam Armature armature
--- @tparam string animation_file_suffix
--- @treturn Clip the exported clip object use sk_definition_writer.reference_to(clip) to reference in other data
local function build_animation_clip(animation, armature, animation_file_suffix)
    local animation_frames, n_frames = build_animation(armature, animation)
    
    sk_definition_writer.add_definition(animation.name .. '_frames', 'struct SKAnimationBoneFrame[]', animation_file_suffix, animation_frames)

    local clip = {
        nFrames = n_frames,
        nBones = #armature.nodes,
        frames = sk_definition_writer.reference_to(animation_frames, 1),
        fps = sk_input.settings.ticks_per_second
    }

    return clip
end

local function build_armature_for_animations(animations)
    local nodes_at_set = {}

    for _, animation in pairs(sk_scene.scene.animations) do
        for _, channel in pairs(animation.channels) do
            nodes_at_set[sk_scene.node_with_name(channel.node_name)] = true
        end
    end

    local all_nodes = {}

    for node, _ in pairs(nodes_at_set) do
        table.insert(all_nodes, node)
    end

    return build_armature(all_nodes)
end

--- @function filter_animations_for_armature
--- @tparam Armature armature
--- @tparam {sk_scene.Animation,...} animations
--- @treturn {sk_scene.Animation,...}
local function filter_animations_for_armature(armature, animations)
    local result = {}

    for _, animation in pairs(animations) do
        for _, channel in pairs(animation.channels) do
            if armature:has_node(sk_scene.node_with_name(channel.node_name)) then
                table.insert(result, animation)
                break
            end
        end
    end

    return result
end

--- @function build_armature_data
--- @tparam Armature armature
--- @tparam sk_definition_writer.RefType|nil gfx_reference_or_nil
--- @tparam string name_hint
--- @tparam string file_suffix
--- @treturn table
local function build_armature_data(armature, gfx_reference_or_nil, name_hint, file_suffix)

    local node_pose = {}
    local transforms = {}
    local parent_mapping = {}

    for _, node in pairs(armature.nodes) do
        add_to_node_pose(node_pose, node)
    
        -- calculate base pose
        local relative_transform = build_node_pose(armature, node, node_pose)

        local pos, rot, scale = relative_transform:decompose()

        table.insert(transforms, {pos * sk_input.settings.fixed_point_scale, rot, scale})

        -- calculate parent mapping
        local _, parent_index = armature:get_parent_bone(node)

        if parent_index then
            table.insert(parent_mapping, parent_index - 1)
        else
            table.insert(parent_mapping, sk_definition_writer.raw('NO_BONE_PARENT'))
        end
    end

    sk_definition_writer.add_definition(name_hint .. '_base_transform', 'struct Transform[]', file_suffix, transforms)
    sk_definition_writer.add_definition(name_hint .. '_parent_mapping', 'unsigned short[]', file_suffix, parent_mapping)

    return {
        displayList = gfx_reference_or_nil or sk_definition_writer.null_value,
        pose = sk_definition_writer.reference_to(transforms, 1),
        numberOfBones = #armature.nodes,
        numberOfAttachments = 0,
        boneParentIndex = sk_definition_writer.reference_to(parent_mapping, 1),
    }
end

--- @type Armature
--- @tfield {sk_scene.Node,...} nodes
Armature.__index = Armature;

--- @function has_node
--- @tparam sk_scene.Node Node
--- @treturn boolean
Armature.has_node = function(armature, node)
    return armature.node_to_index[node] or false
end

--- @function get_parent_bone
--- @tparam sk_scene.Node Node
--- @treturn sk_scene.Node|nil
--- @treturn number bone_index|nil
Armature.get_parent_bone = function(armature, node)
    local curr = node
    
    while curr do
        -- check if current parent is in the armature
        local parent_index = armature.node_to_index[node.parent]
        
        if parent_index then
            -- return the parent bone if in armature
            return armature.nodes[parent_index], parent_index
        end 
        
        curr = curr.parent
    end

    -- no parent bone in armature so return nil
    return nil, nil
end

return {
    build_armature = build_armature,
    build_armature_for_animations = build_armature_for_animations,
    build_armature_data = build_armature_data,
    filter_animations_for_armature = filter_animations_for_armature,
    build_animation_clip = build_animation_clip,
    Armature = Armature,
}