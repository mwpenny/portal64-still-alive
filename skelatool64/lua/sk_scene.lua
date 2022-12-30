--- @module sk_scene

local node_name_cache = nil

local exports

-- Defintions from LuaScene.cpp

---A scene node
---@table Node
---@tfield string name
---@tfield sk_transform.Transform transformation
---@tfield sk_transform.Transform full_transformation
---@tfield Node parent
---@tfield {Node,...} children
---@tfield {sk_mesh.Mesh,...} meshes 

--- Generates mesh and animation data from the current scene
---@function export_default_mesh
---@treturn sk_definition_writer.RawType model
---@treturn sk_definition_writer.RawType material
local function export_default_mesh()
    -- implmentation overridden by LuaScene.cpp
end

-- Defintions from LuaNodeGroups.cpp

---A pairing of nodes and pre parsed node arguments
---@table NodeWithArguments
---@tfield Node node
---@tfield {string,...} arguments the list is strings produced by splitting the node name by spaces

---Returns a list of nodes with the given string prefix
---@function nodes_for_type
---@tparam string prefix the string prefix to search
---@treturn NodeWithArguments
local function nodes_for_type(prefix)
    
end

---Finds a named value in a list of arguments
---@function find_named_argument
---@tfield {string,...} arg_list
---@tfield string name
---@treturn string|nil
local function find_named_argument(arg_list, name)
    for i = 1,#arg_list do
        if (arg_list[i] == name) then
            return arg_list[i + 1]
        end
    end

    return nil
end

---Finds a named value in a list of arguments
---@function find_flag_argument
---@tfield {string,...} arg_list
---@tfield string name
---@treturn boolean
local function find_flag_argument(arg_list, name)
    for i = 1,#arg_list do
        if (arg_list[i] == name) then
            return true
        end
    end

    return false
end

local function build_node_with_name_cache(node)
    node_name_cache[node.name] = node

    for _, child in pairs(node.children) do
        build_node_with_name_cache(child)
    end
end

--- Returns a node with the given name
---@function node_with_name
---@tparam string name
---@treturn Node result
local function node_with_name(name)
    if (not node_name_cache) then
        node_name_cache = {}
        build_node_with_name_cache(exports.scene.root)
    end

    return node_name_cache[name]
end

local function for_each_node(node, callback)
    callback(node)

    for _, child in pairs(node.children) do
        for_each_node(child, callback)
    end
end

---@table Vector3Key
---@tfield number time
---@tfield sk_math.Vector3 value

---@table QuaternionKey
---@tfield number time
---@tfield sk_math.Quaternion value

---@table Channel
---@tfield string node_name
---@tfield {Vector3Key,...} position_keys
---@tfield {QuaternionKey,...} rotation_keys
---@tfield {Vector3Key,...} scaling_keys

---@table Animation
---@tfield string name
---@tfield number duration
---@tfield number ticks_per_second
---@tfield {Channel,...} channels

exports =  {
    export_default_mesh = export_default_mesh,
    nodes_for_type = nodes_for_type,
    find_named_argument = find_named_argument,
    find_flag_argument = find_flag_argument,
    node_with_name = node_with_name,
    for_each_node = for_each_node,
}

return exports