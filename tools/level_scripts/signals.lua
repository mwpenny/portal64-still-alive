local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local yaml_loader = require('tools.level_scripts.yaml_loader')
local util = require('tools.level_scripts.util')

local name_to_index = {}
local name_to_number = {}
local signal_count = 0

local function signal_index_for_name(name)
    local result = name_to_index[name]

    if result then
        return result
    end

    local result = sk_definition_writer.raw(sk_definition_writer.add_macro('SIGNAL_' .. name, tostring(signal_count)))
    name_to_index[name] = result
    name_to_number[name] = signal_count + 1
    signal_count = signal_count + 1
    return result
end

local function signal_number_for_name(name)
    signal_index_for_name(name)
    return name_to_number[name]
end

local function get_signal_count()
    return signal_count
end

local function generate_operator_data(operator)
    return {
        sk_definition_writer.raw(operator.type),
        signal_index_for_name(operator.output),
        {
            signal_index_for_name(operator.input[1]),
            operator.input[2] and signal_index_for_name(operator.input[2]) or -1,
        },
        {
            additionalInputs = {
                operator.input[3] and signal_index_for_name(operator.input[3]) or -1,
                operator.input[4] and signal_index_for_name(operator.input[4]) or -1,
            }
        },
    }
end

local function parse_operation(str_value)
    local pairs = util.string_split(str_value, '=')

    if #pairs ~= 2 then
        error('operators need to have a single equal sign')
    end

    local output = util.trim(pairs[1])

    local inputs = util.string_split(util.trim(pairs[2]), ' ')

    if inputs[1] == 'not' and #inputs == 2 then
        return {
            type = 'SignalOperatorTypeNot',
            output = output,
            input = {inputs[2]},
        }
    end

    if inputs[2] == 'and' and #inputs == 3 then
        return {
            type = 'SignalOperatorTypeAnd',
            output = output,
            input = {inputs[1], inputs[3]},
        }
    end

    if inputs[2] == 'or' and #inputs == 3 then
        return {
            type = 'SignalOperatorTypeOr',
            output = output,
            input = {inputs[1], inputs[3]},
        }
    end

    error('operator must be of the form not a, a and b, a or b')
end

local operators = {}

for _, operation in pairs(yaml_loader.json_contents.operators or {}) do
    table.insert(operators, generate_operator_data(parse_operation(operation)))
end

sk_definition_writer.add_definition('signal_operations', 'struct SignalOperator[]', '_geo', operators)

return {
    signal_index_for_name = signal_index_for_name,
    signal_number_for_name = signal_number_for_name,
    get_signal_count = get_signal_count,
    operators = operators,
}