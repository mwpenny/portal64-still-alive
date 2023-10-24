
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local room_export = require('tools.level_scripts.room_export')
local signals = require('tools.level_scripts.signals')
local animation = require('tools.level_scripts.animation')
local yaml_loader = require('tools.level_scripts.yaml_loader')
local util = require('tools.level_scripts.util')

sk_definition_writer.add_header('"../build/src/audio/clips.h"')

local function cutscene_index(cutscenes, name)
    for _, cutscene in pairs(cutscenes) do
        if cutscene.name == name then
            return cutscene.macro
        end
    end

    return -1
end

local function generate_locations()
    local result = {}
    local location_data = {}

    for _, location in pairs(sk_scene.nodes_for_type("@location")) do
        local position, rotation, scale = location.node.full_transformation:decompose()

        local room_index = room_export.node_nearest_room_index(location.node)

        local name = location.arguments[1] or ''

        table.insert(result, {
            name = name,
            room_index = room_index,
            macro = sk_definition_writer.raw(sk_definition_writer.add_macro('LOCATION_' .. name, #result)),
        })

        table.insert(location_data, {
            {
                position = position,
                rotation = rotation,
                scale = scale,
            },
            roomIndex = room_index,
        })
    end

    sk_definition_writer.add_definition("locations", "struct Location[]", "_geo", location_data)
    
    return result, location_data
end

local locations, location_data = generate_locations()

local function find_location_index(name)
    for _, location in pairs(locations) do
        if location.name == name then
            return location.macro
        end
    end

    print('Could not find a location with the name ' .. name)

    return 0
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

local function string_starts_with(str, prefix)
    return string.sub(str, 1, #prefix) == prefix
end

local function generate_cutscene_step(cutscene_name, step, step_index, label_locations, cutscenes)
    local result = {}

    if step.command == "play_sound" or step.command == "start_sound" and #step.args >= 1 then
        result.type = step.command == "play_sound" and 
            sk_definition_writer.raw('CutsceneStepTypePlaySound') or 
            sk_definition_writer.raw('CutsceneStepTypeStartSound')
        result.playSound = {
            sk_definition_writer.raw(string_starts_with(step.args[1], "SOUNDS_") and step.args[1] or ("SOUNDS_" .. step.args[1])),
            tonumber(step.args[2] or "1") * 255,
            math.floor(tonumber(step.args[3] or "1") * 64 + 0.5),
        }
    elseif step.command == "q_sound" and #step.args >= 3 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeQueueSound')
        result.queueSound = {
            sk_definition_writer.raw(string_starts_with(step.args[1], "SOUNDS_") and step.args[1] or ("SOUNDS_" .. step.args[1])),
            sk_definition_writer.raw(step.args[2]),
            sk_definition_writer.raw(step.args[3]),
            tonumber(step.args[4] or "1") * 255,
        }
    elseif step.command == "wait_for_channel" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeWaitForChannel')
        result.waitForChannel = {
            sk_definition_writer.raw(step.args[1]),
        }
    elseif step.command == "delay" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeDelay')
        result.delay = tonumber(step.args[1])
    elseif step.command == "open_portal" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeOpenPortal')
        result.openPortal = {
            find_location_index(step.args[1]),
            step.args[2] == "1" and 1 or 0,
            step.args[3] == "pedestal" and 1 or 0
        }
    elseif step.command == "close_portal" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeClosePortal')
        result.closePortal = {
            step.args[1] == "1" and 1 or 0,
        }
    elseif (step.command == "set_signal" or step.command == "clear_signal") and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeSetSignal')
        result.setSignal = {
            signals.signal_index_for_name(step.args[1]),
            step.command == 'set_signal' and 1 or 0,
        }
    elseif step.command == "wait_for_signal" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeWaitForSignal')
        result.waitForSignal = {
            signals.signal_index_for_name(step.args[1]),
        }
    elseif step.command == "teleport_player" and #step.args >= 2 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeTeleportPlayer')
        result.teleportPlayer = {
            find_location_index(step.args[1]),
            find_location_index(step.args[2]),
        }
    elseif step.command == "load_level" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeLoadLevel')
        result.loadLevel = {
            find_location_index(step.args[1]),
            -- -1 means next level
            -1,
        }
    elseif step.command == "goto" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeGoto')

        local label_location = label_locations[step.args[1]]

        if not label_location then
            error("Unrecognized label '" .. step.args[1] .. "' in cutscene " .. cutscene_name)
        end

        result.gotoStep = {
            label_location - step_index,
        }
    elseif step.command == "start_cutscene" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeStartCutscene')
        result.cutscene = {
            cutscene_index(cutscenes, step.args[1]),
        }
    elseif step.command == "stop_cutscene" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeStopCutscene')
        result.cutscene = {
            cutscene_index(cutscenes, step.args[1]),
        }
    elseif step.command == "wait_for_cutscene" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypeWaitForCutscene')
        result.cutscene = {
            cutscene_index(cutscenes, step.args[1]),
        }
    elseif step.command == "hide_pedestal" then
        result.type = sk_definition_writer.raw('CutsceneStepTypeHidePedestal')
    elseif step.command == "point_pedestal" and #step.args >= 1 then
        result.type = sk_definition_writer.raw('CutsceneStepTypePointPedestal')
        result.pointPedestal = {
            find_location_index(step.args[1]),
        }
    elseif step.command == "play_animation" then
        result.type = sk_definition_writer.raw('CutsceneStepPlayAnimation')
        local armature = animation.get_armature_index_with_name(step.args[1])
        local animation = animation.get_animation_with_name(step.args[1], step.args[2])

        if not armature then
            error("Unrecognized animator " .. step.args[1])
        end

        if not animation then
            error("Unrecognized animation " .. step.args[2])
        end

        result.playAnimation = {
            armature,
            animation,
            step.args[3] and (tonumber(step.args[3]) * 127) or 127,
        }
    elseif step.command == "pause_animation" then
        result.type = sk_definition_writer.raw('CutsceneStepSetAnimationSpeed')
        result.setAnimationSpeed = {
            animation.get_armature_index_with_name(step.args[1]) or 0,
            0,
        }
    elseif step.command == "resume_animation" then
        result.type = sk_definition_writer.raw('CutsceneStepSetAnimationSpeed')
        result.setAnimationSpeed = {
            animation.get_armature_index_with_name(step.args[1]) or 0,
            step.args[2] and (tonumber(step.args[2]) * 127) or 127,
        }
    elseif step.command == "wait_for_animation" then
        result.type = sk_definition_writer.raw('CutsceneStepWaitForAnimation')
        result.waitForAnimation = {
            animation.get_armature_index_with_name(step.args[1]) or 0,
        }
    elseif step.command == "save_checkpoint" then
        result.type = sk_definition_writer.raw('CutsceneStepSaveCheckpoint')
    elseif step.command == "kill_player" then
        result.type = sk_definition_writer.raw('CutsceneStepKillPlayer')
        result.killPlayer = {
            step.args[1] == 'water' and 1 or 0,
        }
    elseif step.command == "show_prompt" then
        result.type = sk_definition_writer.raw('CutsceneStepShowPrompt')
        result.showPrompt = {
            sk_definition_writer.raw(step.args[1]),
        }
    elseif step.command == "rumble" then
        result.type = sk_definition_writer.raw('CutsceneStepRumble')
        result.rumble = {
            tonumber(step.args[1]),
        }
    else
        error("Unrecognized cutscene step " .. step.command)
        result.type = sk_definition_writer.raw('CutsceneStepTypeNoop')
        table.noop = 0;
    end

    return result
end

local function generate_cutscenes()
    local cutscenes_result = {}
    local cutscene_data = {}

    local cutscene_json = yaml_loader.json_contents.cutscenes or {}

    local cutscenes = {}

    for cutscene_name, cutscene_steps in pairs(cutscene_json) do
        table.insert(cutscenes, {
            name = cutscene_name,
            steps = cutscene_steps,
            macro = sk_definition_writer.raw(sk_definition_writer.add_macro("CUTSCENE_" .. cutscene_name, #cutscenes)),
        })
    end

    for _, cutscene in pairs(cutscenes) do
        local other_steps = {}

        for _, step_string in pairs(cutscene.steps) do
            local args = util.string_split(step_string, ' ')

            table.insert(other_steps, {
                command = args[1],
                args = {table.unpack(args, 2)},
            })
        end

        local label_locations = find_label_locations(other_steps)

        local steps = {}

        for step_index, step in pairs(other_steps) do
            table.insert(steps, generate_cutscene_step(cutscene.name, step, step_index, label_locations, cutscenes))
        end

        sk_definition_writer.add_definition(cutscene.name .. '_steps', 'struct CutsceneStep[]', '_geo', steps)

        table.insert(cutscenes_result, {
            name = cutscene.name,
            steps = steps,
            macro = sk_definition_writer.raw(sk_definition_writer.add_macro("CUTSCENE_" .. cutscene.name, #cutscenes_result)),
        })

        table.insert(cutscene_data, {
            sk_definition_writer.reference_to(steps, 1),
            #steps,
        })
    end

    return cutscenes_result, cutscene_data
end

local function parse_trigger_signal(signal)
    if not signal or signal == '-1' then
        return -1
    end

    return signals.signal_index_for_name(signal)
end

local function signal_type_index(index, is_hover)
    if index == 3 then
        if is_hover then
            return sk_definition_writer.raw('ObjectTriggerTypeCubeHover')
        end

        return sk_definition_writer.raw('ObjectTriggerTypeCube')
    end

    return sk_definition_writer.raw('ObjectTriggerTypePlayer')
end

local function generate_triggers(cutscenes)
    local result = {}
    
    for _, trigger in pairs(sk_scene.nodes_for_type('@trigger')) do
        local first_mesh = trigger.node.meshes[1]
        
        local triggers = {}
        
        for i = 1, #trigger.arguments, 2 do
            local cutscene_name = trigger.arguments[i]
            local cutscene = cutscene_index(cutscenes, cutscene_name)
            table.insert(triggers, {
                signal_type_index(i, string.sub(cutscene_name, 1, 6) == "HOVER_"),
                cutscene,
                parse_trigger_signal(trigger.arguments[i + 1]),
            })
        end

    
        if first_mesh then
            local transformed = first_mesh:transform(trigger.node.full_transformation)
    
            table.insert(result, {
                transformed.bb,
                sk_definition_writer.reference_to(triggers, 1),
                #triggers,
            })

            sk_definition_writer.add_definition("trigger_targets", "struct ObjectTriggerInfo[]", "_geo", triggers)
        end
    end

    return result
end
local cutscenes, cutscene_data = generate_cutscenes()
local triggers = generate_triggers(cutscenes)

sk_definition_writer.add_definition("triggers", "struct Trigger[]", "_geo", triggers)
sk_definition_writer.add_definition("cutscenes", "struct Cutscene[]", "_geo", cutscene_data)

local function find_cutscene_index(name)
    return cutscene_index(cutscenes, name)
end

return {
    triggers = triggers,
    cutscene_data = cutscene_data,
    location_data = location_data,
    find_location_index = find_location_index,
    find_cutscene_index = find_cutscene_index,
}