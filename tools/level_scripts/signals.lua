local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')

local name_to_index = {}
local signal_count = 0

local function signal_index_for_name(name)
    local result = name_to_index[name]

    if result then
        return result
    end

    local result = sk_definition_writer.raw(sk_definition_writer.add_macro('SIGNAL_' .. name, tostring(signal_count)))
    name_to_index[name] = result
    signal_count = signal_count + 1
    return result
end

local function determine_signal_order(operators, result, used_signals, signal_producers, operation_index)
    -- check if the signal has already been added
    if used_signals[operation_index] then
        return
    end

    used_signals[operation_index] = true

    local input_signal = operators[operation_index]

    for _, signal_name in pairs(input_signal.input) do
        for _, producer_index in pairs(signal_producers[signal_name] or {}) do
            determine_signal_order(operators, result, used_signals, signal_producers, operation_index)
        end
    end

    table.insert(result, input_signal)
end

local function order_signals(operators)
    local signal_producers = {}

    for index, operation in pairs(operators) do
        if signal_producers[operation.output] then
            table.insert(signal_producers[operation.output], index)
        else
            signal_producers[operation.output] = {index}
        end
    end

    local result = {}
    local used_signals = {}

    for operation_index = 1,#operators do
        determine_signal_order(operators, result, used_signals, signal_producers, operation_index)
    end

    return result
end

local unordered_operators = {}

for _, and_operator in pairs(sk_scene.nodes_for_type('@and')) do
    table.insert(unordered_operators, {
        type = 'SignalOperatorTypeAnd',
        output = and_operator.arguments[1],
        input = {table.unpack(and_operator.arguments, 2)},
    })
end

for _, or_operator in pairs(sk_scene.nodes_for_type('@or')) do
    table.insert(unordered_operators, {
        type = 'SignalOperatorTypeOr',
        output = or_operator.arguments[1],
        input = {table.unpack(or_operator.arguments, 2)},
    })
end

for _, not_operator in pairs(sk_scene.nodes_for_type('@not')) do
    table.insert(unordered_operators, {
        type = 'SignalOperatorTypeNot',
        output = not_operator.arguments[1],
        input = {not_operator.arguments[2]},
    })
end

local ordered_operators = order_signals(unordered_operators)

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

local operators = {}

for _, operation in pairs(ordered_operators) do
    table.insert(operators, generate_operator_data(operation))
end

sk_definition_writer.add_definition('signal_operations', 'struct SignalOperator[]', '_geo', operators)

return {
    signal_index_for_name = signal_index_for_name,
    operators = operators,
}