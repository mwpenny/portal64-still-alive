
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')

sk_definition_writer.add_header('"../build/src/audio/clips.h"')


local function does_belong_to_cutscene(first_step, step)
    local offset = step.position - first_step.position
    local local_pos = first_step.rotation * offset

    return local_pos.y >= 0 and local_pos.x * local_pos.x + local_pos.z * local_pos.z < 0.1
end

local function distance_from_start(first_step, step)
    local offset = step.position - first_step.position
    local local_pos = first_step.rotation * offset
    return local_pos.y
end

local function cutscene_index(cutscenes, name)
    for index, cutscene in pairs(cutscenes) do
        if cutscene.name == name then
            return index - 1
        end
    end

    return -1
end

local function find_label_locations(steps)
    local result = {}
    local current_index = 1
    
    while current_index <= #steps do
        local step = steps[current_index]

        if step.command == "label" and #step.args >= 1 then
            result[step.args[1]] = current_index
            table.remove(steps, current_index)
        else
            current_index = current_index + 1
        end
    end

    return result
end

local function generate_cutscene_step(step)
    local result = {}

    if step.command == "play_sound" or step.command == "start_sound" and #step.args >= 1 then
        result.type = step.command == "play_sound" and 
            sk_definition_writer.raw('CutsceneStepTypePlaySound') or 
            sk_definition_writer.raw('CutsceneStepTypeStartSound')
    else
        result.type = sk_definition_writer.raw('CutsceneStepTypeNoop')
        table.noop = 0;
    end

    return result
end

local function generate_cutscenes()
    local step_nodes = sk_scene.nodes_for_type("@cutscene")

    local steps = {}
    local cutscenes = {}

    for _, node_info in pairs(step_nodes) do
        if #node_info.arguments > 0 then
            local command = node_info.arguments[1]
            local args = {table.unpack(node_info.arguments, 2)}

            local scale, rotation, position = node_info.node.transformation:decompose()

            local step = {
                command = command,
                args = args,
                position = position,
                rotation = rotation:conjugate(),
            }

            if command == "start" and #args > 1 then
                table.insert(cutscenes, {
                    name = args[1],
                    steps = {step},
                })
            else
                table.insert(steps, step)
            end
        end
    end

    for _, cutscene in pairs(cutscenes) do
        for _, step in pairs(steps) do
            if does_belong_to_cutscene(cutscene.steps[1], step) then
                table.insert(cutscene.steps, step)
            end
        end
    end

    local cutscenes_result = {}

    for _, cutscene in pairs(cutscenes) do
        local first_step = cutscene.steps[1]
        local other_steps = {table.unpack(cutscene.steps, 2)}

        table.sort(other_steps, function(a, b)
            return distance_from_start(first_step, a) < distance_from_start(first_step, b)
        end)

        local label_locations = find_label_locations(other_steps)

        local steps = {}

        for step_index, step in pairs(other_steps) do
            table.insert(steps, generate_cutscene_step())
        end

        sk_definition_writer.add_definition(cutscene.name .. '_steps', 'struct CutsceneStep[]', '_geo', steps)

        table.insert(cutscenes_result, {
            name = cutscene.name,
            steps = steps,
        })
    end

    return cutscenes_result
end

local function generate_triggers(cutscenes)
    local result = {}
    
    for _, trigger in pairs(sk_scene.nodes_for_type('@trigger')) do
        local first_mesh = trigger.node.meshes[1]
        local cutscene_index = cutscene_index(cutscenes, trigger.arugments[1])
    
        if first_mesh and cutscene_index ~= -1 then
            local transformed = first_mesh:transform(trigger.node.full_transformation)
    
            table.insert(result, {
                transformed.bb,
                cutscene_index,
            })
        end
    end

    return result
end

local cutscenes = generate_cutscenes()
local triggers = generate_triggers(cutscenes)

sk_definition_writer.add_definition("triggers", "struct Trigger[]", "_geo", triggers)

return {
    triggers = triggers,
}