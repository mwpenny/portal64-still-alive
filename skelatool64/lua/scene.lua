
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

function export_default_mesh()
    local allNodes = {}


    for k, value in pairs(nodes_for_type("")) do
        local renderChunks = generate_render_chunks(value.node)
    
        for _, toAdd in pairs(renderChunks) do
            table.insert(allNodes, toAdd)
        end
    end
    
    generate_mesh(allNodes, "_geo")
end