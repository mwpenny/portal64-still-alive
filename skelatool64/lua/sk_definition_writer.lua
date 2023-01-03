--- @module sk_definition_writer

local exports = {}

-- From LuaDefinitionWriter.cpp

---@function add_header
---@tparam string include
exports.add_header = function(include)
    -- implmentation in LuaDefinitionWriter.cpp
end

---@function add_macro
---@tparam string name a hint on how to name the macro
---@tparam string value the value of the macro
---@treturn string the final name for the macro
exports.add_macro = function(name, value)
    -- implmentation in LuaDefinitionWriter.cpp
end

local pending_definitions = {}

--- @table RefType
local RefType = {}

--- creates a pointer to another piece of data
--- the data being referenced must be added to the output
--- via add_definition
---@function reference_to
---@tparam any value the value to reference
---@tparam[opt] integer index if value is an array, you can specify the element to reference using this index
---@treturn RefType result
local function reference_to(value, index)
    return setmetatable({ value = value, index = index }, RefType)
end

exports.reference_to = reference_to

--- returns true if value is a reference
---@function is_reference_type
---@tparam any value any
---@treturn boolean result
local function is_reference_type(value)
    return getmetatable(value) == RefType
end

exports.is_reference_type = is_reference_type

--- @table RawType
local RawType = {}

local function raw(value)
    return setmetatable({ value = value}, RawType)
end

RawType.__index = RawType;

function RawType.__tostring(raw)
    return 'raw(' .. raw.value .. ')'
end

--- renders a string directly in the ouptut instead of wrapping the output in quotes
---@function raw
---@tparam string value
---@treturn RawType result
exports.raw = raw

--- returns true if value is a RawType
---@function is_raw
---@tparam any value
---@treturn boolean result
local function is_raw(value)
    return getmetatable(value) == RawType
end

exports.is_raw = is_raw

--- alias for raw("NULL")
--- @table null_value
local null_value = raw("NULL")

exports.null_value = null_value

--- @table MacroType
local MacroType = {}

--- Generates as a macro eg `macro("MACRO_NAME", 1, 2)` will be displayed as
--- MACRO_NAME(1, 2) in the c file output
---@function macro
---@tparam string name
---@tparam {any,...} ...
---@treturn MacroType result
local function macro(name, ...)
    if (type(name) ~= "string") then
        error("name should be of type string got " .. type(name), 2)
    end

    return setmetatable({ name = name, args = {...}}, MacroType)
end

exports.macro = macro

--- Returns true if value is of type MacroType
---@function is_macro
---@tparam any value
---@treturn boolean
local function is_macro(value)
    return getmetatable(value) == MacroType
end

exports.is_macro = is_macro

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

--- Outputs a c file defintion 
---@function add_definition
---@tparam string nameHint
---@tparam string dataType the c type the definition is, if it is an array it should end in []
---@tparam string location the file suffix where this definiton should be located in
---@tparam any data The data of the file definition
local function add_definition(nameHint, dataType, location, data)
    if (type(nameHint) ~= "string") then
        error("nameHint should be a string", 2)
    end

    if (type(dataType) ~= "string") then
        error("dataType should be a string", 2)
    end

    if (type(location) ~= "string") then
        error("location should be a string", 2)
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

exports.add_definition = add_definition

local function populate_name_mapping(path, object, result)
    if (type(object) ~= "table") then
        return
    end

    if (is_reference_type(object) or is_macro(object) or is_raw(object)) then
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

---@table PendingDefinition
---@tfield string nameHint 
---@tfield string dataType 
---@tfield string location 
---@tfield any data 

--- Returns and clears all definitions that have been created using add_definiton
--- meant for use in the c code
---@function consume_pending_definitions
---@treturn {PendingDefinition,...} result
local function consume_pending_definitions()
    local result = pending_definitions
    consume_pending_definitions = {}
    return result
end

exports.consume_pending_definitions = consume_pending_definitions

--- Processes definitions correctly connecting references
--- meant for use in the c code
---@function process_definitions
---@tparam {PendingDefinition,...} definitions
local function process_definitions(definitions)
    local name_mapping = {}
    
    for k, v in pairs(definitions) do
        populate_name_mapping(v.name, v.data, name_mapping)
    end

    for k, v in pairs(definitions) do
        v.data = replace_references(v.data, name_mapping, v.name)
    end
end

exports.process_definitions = process_definitions

return exports