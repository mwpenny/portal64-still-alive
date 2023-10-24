
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_math = require('sk_math')
local room_export = require('tools.level_scripts.room_export')
local trigger = require('tools.level_scripts.trigger')
local world = require('tools.level_scripts.world')
local signals = require('tools.level_scripts.signals')

local box_droppers = {}

for _, dropper in pairs(sk_scene.nodes_for_type('@box_dropper')) do
    local position = dropper.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(dropper.node)

    table.insert(box_droppers, {
        position,
        room_index,
        signals.signal_index_for_name(dropper.arguments[1] or ''),
    })
end

sk_definition_writer.add_definition('box_dropper', 'struct BoxDropperDefinition[]', '_geo', box_droppers)

local buttons = {}

for _, button in pairs(sk_scene.nodes_for_type('@button')) do
    local position = button.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(button.node)

    table.insert(buttons, {
        position,
        room_index,
        signals.signal_index_for_name(button.arguments[1] or ''),
        signals.signal_index_for_name(button.arguments[2] or ''),
    })
end

sk_definition_writer.add_definition('buttons', 'struct ButtonDefinition[]', '_geo', buttons)

local decor = {}

for _, decor_entry in pairs(sk_scene.nodes_for_type('@decor')) do
    local position, rotation = decor_entry.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(decor_entry.node)

    table.insert(decor, {
        position,
        rotation,
        room_index,
        sk_definition_writer.raw('DECOR_TYPE_' .. decor_entry.arguments[1]),
    })
end

sk_definition_writer.add_definition('decor', 'struct DecorDefinition[]', '_geo', decor)
sk_definition_writer.add_header('"decor/decor_object_list.h"')

local doors = {}

local function parse_door_type(name)
    if name == '02' then
        return sk_definition_writer.raw('DoorType02')
    end

    return sk_definition_writer.raw('DoorType01')
end

for _, door in pairs(sk_scene.nodes_for_type('@door')) do
    local position, rotation = door.node.full_transformation:decompose()

    table.insert(doors, {
        position,
        rotation,
        world.find_coplanar_doorway(position) - 1,
        signals.signal_index_for_name(door.arguments[1] or ''),
        parse_door_type(door.arguments[2])
    })
end

sk_definition_writer.add_definition('doors', 'struct DoorDefinition[]', '_geo', doors)

local elevators = {}

local elevator_nodes = sk_scene.nodes_for_type('@elevator')

for _, elevator in pairs(elevator_nodes) do
    local position, rotation = elevator.node.full_transformation:decompose()

    local target_elevator = -1

    if elevator.arguments[2] == 'next_level' then
        target_elevator = #elevator_nodes
    else
        for other_index, other_elevator in pairs(elevator_nodes) do
            if other_elevator.arguments[1] == elevator.arguments[2] then
                target_elevator = other_index - 1
                break
            end
        end
    end

    local room_index = room_export.node_nearest_room_index(elevator.node)

    table.insert(elevators, {
        position,
        rotation,
        room_index,
        target_elevator,
    })
end

sk_definition_writer.add_definition('elevators', 'struct ElevatorDefinition[]', '_geo', elevators)

local fizzlers = {}

for _, fizzler in pairs(sk_scene.nodes_for_type('@fizzler')) do
    local position, rotation = fizzler.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(fizzler.node)

    table.insert(fizzlers, {
        position,
        rotation,
        1,
        1,
        room_index,
    })
end

sk_definition_writer.add_definition('fizzlers', 'struct FizzlerDefinition[]', '_geo', fizzlers)

local pedestals = {}

for _, pedestal in pairs(sk_scene.nodes_for_type('@pedestal')) do
    local position = pedestal.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(pedestal.node)

    table.insert(pedestals, {
        position,
        room_index,
    })
end

sk_definition_writer.add_definition('pedestals', 'struct PedestalDefinition[]', '_geo', pedestals)

local signage = {}

for _, signage_element in pairs(sk_scene.nodes_for_type('@signage')) do
    local position, rotation = signage_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(signage_element.node)

    table.insert(signage, {
        position,
        rotation,
        room_index,
        sk_definition_writer.raw(signage_element.arguments[1]),
    })
end

sk_definition_writer.add_definition('signage', 'struct SignageDefinition[]', '_geo', signage)


local switches = {}

for _, switch_element in pairs(sk_scene.nodes_for_type('@switch')) do
    local position, rotation = switch_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(switch_element.node)

    table.insert(switches, {
        position,
        rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        room_index,
        signals.signal_index_for_name(switch_element.arguments[1]),
        switch_element.arguments[2] and tonumber(switch_element.arguments[2]) or 0,
    })
end

sk_definition_writer.add_definition('switches', 'struct SwitchDefinition[]', '_geo', switches)

local ball_launchers = {}

for _, switch_element in pairs(sk_scene.nodes_for_type('@ball_launcher')) do
    local position, rotation = switch_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(switch_element.node)

    table.insert(ball_launchers, {
        position,
        rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        room_index,
        signals.signal_index_for_name(switch_element.arguments[1]),
        switch_element.arguments[2] and tonumber(switch_element.arguments[2]) or 10,
        switch_element.arguments[3] and tonumber(switch_element.arguments[3]) or 3
    })
end

sk_definition_writer.add_definition('ball_launchers', 'struct BallLauncherDefinition[]', '_geo', ball_launchers)

local ball_catchers = {}

for _, switch_element in pairs(sk_scene.nodes_for_type('@ball_catcher')) do
    local position, rotation = switch_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(switch_element.node)

    table.insert(ball_catchers, {
        position,
        rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        room_index,
        signals.signal_index_for_name(switch_element.arguments[1]),
    })
end

sk_definition_writer.add_definition('ball_catchers', 'struct BallCatcherDefinition[]', '_geo', ball_catchers)

local clocks = {}

for _, clock_element in pairs(sk_scene.nodes_for_type('@clock')) do
    local position, rotation = clock_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(clock_element.node)

    table.insert(clocks, {
        position,
        rotation,
        room_index,
        tonumber(clock_element.arguments[1]),
    })
end

sk_definition_writer.add_definition('clocks', 'struct ClockDefinition[]', '_geo', clocks)

local security_cameras = {}

for _, security_camera_element in pairs(sk_scene.nodes_for_type('@security_camera')) do
    local position, rotation = security_camera_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(security_camera_element.node)

    table.insert(security_cameras, {
        position,
        rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        room_index,
    })
end

sk_definition_writer.add_definition('security_cameras', 'struct SecurityCameraDefinition[]', '_geo', security_cameras)

return {
    box_droppers = box_droppers,
    buttons = buttons,
    decor = decor,
    doors = doors,
    elevators = elevators,
    fizzlers = fizzlers,
    pedestals = pedestals,
    signage = signage,
    switches = switches,
    ball_catchers = ball_catchers,
    ball_launchers =  ball_launchers,
    clocks = clocks,
    security_cameras = security_cameras,
}