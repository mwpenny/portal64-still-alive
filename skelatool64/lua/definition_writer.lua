
local pending_definitions = {}

local RefType = {}

function reference_to(value, index)
    return setmetatable({ value = value, index = index }, RefType)
end

function is_reference_type(value)
    return getmetatable(value) == RefType
end

local RawType = {}

function raw(value)
    return setmetatable({ value = value}, RawType)
end

function is_raw(value)
    return getmetatable(value) == RawType
end


null_value = raw("NULL")

local MacroType = {}

function macro(name, ...)
    if (type(name) ~= "string") then
        error("name should be of type string got " .. type(name))
    end

    return setmetatable({ name = name, args = {...}}, MacroType)
end

function is_macro(value)
    return getmetatable(value) == MacroType
end

local function validate_definition(data, visited, name_path)
    if (visited[data]) then
        error("Circular reference found '" .. name_path .. "'")
        return false
    end

    local data_type = type(data)

    if (data_type == "nil" or data_type == "boolean" or data_type == "number" or data_type == "string") then
        return true
    end

    if (data_type == "function" or data_type == "userdata" or data_type == "thread") then
        error("Cannot use '" .. data_type .. "' in a c file definition for path '" .. name_path .. "'")
        return false
    end

    if (is_reference_type(data) or is_raw(data)) then
        return true
    end

    visited[data] = true    

    for k, v in pairs(data) do 
        local key_type = type(k)

        if (key_type ~= "string" and key_type ~= "number") then
            error("Cannot use '" .. data_type .. "' as a key in a c file definiton for path '" .. name_path .. "'")
            return false
        end

        local name

        if (key_type == "number") then
            name = name_path .. "[" .. (k - 1) .. "]"
        else
            name = name_path .. "." .. k
        end

        if (not validate_definition(v, visited, name)) then
            return false
        end
    end

    visited[data] = nil

    return true
end

function add_definition(nameHint, dataType, location, data)
    if (type(nameHint) ~= "string") then
        error("nameHint should be a string")
    end

    if (type(dataType) ~= "string") then
        error("dataType should be a string")
    end

    if (type(location) ~= "string") then
        error("location should be a string")
    end

    if (not validate_definition(data, {}, nameHint)) then
        return false
    end

    table.insert(pending_definitions, {
        nameHint = nameHint,
        dataType = dataType,
        location = location,
        data = data
    })

    return true
end

local function populate_name_mapping(path, object, result)
    if (type(object) ~= "table") then
        return
    end

    if (is_reference_type(object) or is_macro(object)) then
        return
    end

    if (result[object]) then
        error("Path already set for object " .. result[object] .. " with new path " .. path)
    end

    result[object] = path

    for k, v in pairs(object) do
        if type(k) == "number" then
            populate_name_mapping(path .. "[" .. (k - 1) .. "]", v, result)
        else
            populate_name_mapping(path .. "." .. k, v, result)
        end
    end
end

local function replace_references(object, name_mapping, name_path)
    if type(object) ~= "table" then
        return object
    end

    if (is_reference_type(object)) then
        if (object.value == nil) then
            return null_value
        end

        local reference_name = name_mapping[object.value]

        if (reference_name == nil) then
            error("A reference '" .. name_path .. "' was used on an object not exported in a definition")
        end

        if object.index then
            reference_name = reference_name .. '[' .. (object.index - 1) .. ']'
        end

        return raw("&" .. reference_name)
    end

    local changes = {}
    local hasChange = {}
    local hasChanges = false

    for k, v in pairs(object) do
        local name

        if (type(k) == "number") then
            name = name_path .. "[" .. (k - 1) .. "]"
        else
            name = name_path .. "." .. k
        end

        local replacement = replace_references(v, name_mapping, name)

        if (replacement ~= v) then
            changes[k] = replacement
            hasChange[k] = true
            hasChanges = true
        end
    end

    if (not hasChanges) then
        return object
    end

    local result = {}

    for k, v in pairs(object) do
        if (hasChange[k]) then
            result[k] = changes[k]
        else
            result[k] = object[k]
        end
    end

    return result
end

function consume_pending_definitions()
    local result = pending_definitions
    consume_pending_definitions = {}
    return result
end

function process_definitions(definitions)
    local name_mapping = {}
    
    for k, v in pairs(definitions) do
        print("populating name mapping for " .. v.name)
        populate_name_mapping(v.name, v.data, name_mapping)
    end

    for k, v in pairs(definitions) do
        print("replacing references for " .. v.name)
        v.data = replace_references(v.data, name_mapping, v.name)
    end
end