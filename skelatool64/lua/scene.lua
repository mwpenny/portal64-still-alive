
local node_name_cache = nil

local function build_node_with_name_cache(node)
    node_name_cache[node.name] = node

    for _, child in pairs(node.children) do
        build_node_with_name_cache(child)
    end
end

function node_with_name(name)
    if (not node_name_cache) then
        node_name_cache = {}
        build_node_with_name_cache(scene.root)
    end

    return node_name_cache[name]
end