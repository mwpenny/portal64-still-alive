
local sk_definition_writer = require('sk_definition_writer')
local sk_scene = require('sk_scene')
local sk_math = require('sk_math')
local room_export = require('tools.level_scripts.room_export')
local trigger = require('tools.level_scripts.trigger')
local signals = require('tools.level_scripts.signals')

local box_droppers = {}

for _, dropper in pairs(sk_scene.nodes_for_type('@box_dropper')) do
    local position = dropper.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(dropper.node)

    local cube_type = dropper.arguments[2] or 'Standard'

    table.insert(box_droppers, {
        position = position,
        roomIndex = room_index,
        signalIndex = signals.signal_index_for_name(dropper.arguments[1]),
        cubeType = sk_definition_writer.raw('BoxDropperCubeType' .. cube_type),
    })
end

sk_definition_writer.add_definition('box_dropper', 'struct BoxDropperDefinition[]', '_geo', box_droppers)

local buttons = {}

for _, button in pairs(sk_scene.nodes_for_type('@button')) do
    local position = button.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(button.node)

    table.insert(buttons, {
        location = position,
        roomIndex = room_index,
        signalIndex = signals.signal_index_for_name(button.arguments[1]),
        objectSignalIndex = signals.optional_signal_index_for_name(button.arguments[2])
    })
end

sk_definition_writer.add_definition('buttons', 'struct ButtonDefinition[]', '_geo', buttons)

local decor = {}

for _, decor_entry in pairs(sk_scene.nodes_for_type('@decor')) do
    local position, rotation = decor_entry.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(decor_entry.node)

    table.insert(decor, {
        position = position,
        rotation = rotation,
        roomIndex = room_index,
        decorId = sk_definition_writer.raw('DECOR_TYPE_' .. decor_entry.arguments[1]),
        startAsleep = sk_scene.find_flag_argument(decor_entry.arguments, "start_asleep")
    })
end

sk_definition_writer.add_definition('decor', 'struct DecorDefinition[]', '_geo', decor)
sk_definition_writer.add_header('"decor/decor_object_list.h"')

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
        position = position,
        rotation = rotation,
        roomIndex = room_index,
        targetElevator = target_elevator,
    })
end

sk_definition_writer.add_definition('elevators', 'struct ElevatorDefinition[]', '_geo', elevators)

local fizzlers = {}

for _, fizzler in pairs(sk_scene.nodes_for_type('@fizzler')) do
    local position, rotation, scale = fizzler.node.full_transformation:decompose()
    local bounding_box = fizzler.node.meshes[1].bb
    local width = (bounding_box.max.x - bounding_box.min.x) * scale.x * 0.5
    local height = (bounding_box.max.y - bounding_box.min.y) * scale.y * 0.5

    local room_index = room_export.node_nearest_room_index(fizzler.node)

    table.insert(fizzlers, {
        position = position,
        rotation = rotation,
        width = width,
        height = height,
        roomIndex = room_index,
        cubeSignalIndex = signals.optional_signal_index_for_name(fizzler.arguments[1]),
    })
end

sk_definition_writer.add_definition('fizzlers', 'struct FizzlerDefinition[]', '_geo', fizzlers)

local pedestals = {}

for _, pedestal in pairs(sk_scene.nodes_for_type('@pedestal')) do
    local position = pedestal.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(pedestal.node)

    table.insert(pedestals, {
        position = position,
        roomIndex = room_index,
    })
end

sk_definition_writer.add_definition('pedestals', 'struct PedestalDefinition[]', '_geo', pedestals)

local signage = {}

for _, signage_element in pairs(sk_scene.nodes_for_type('@signage')) do
    local position, rotation = signage_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(signage_element.node)

    table.insert(signage, {
        position = position,
        rotation = rotation,
        roomIndex = room_index,
        testChamberNumber = sk_definition_writer.raw(signage_element.arguments[1]),
    })
end

sk_definition_writer.add_definition('signage', 'struct SignageDefinition[]', '_geo', signage)


local switches = {}

for _, switch_element in pairs(sk_scene.nodes_for_type('@switch')) do
    local position, rotation = switch_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(switch_element.node)

    table.insert(switches, {
        location = position,
        rotation = rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        roomIndex = room_index,
        signalIndex = signals.signal_index_for_name(switch_element.arguments[1]),
        duration = switch_element.arguments[2] and tonumber(switch_element.arguments[2]) or 0,
    })
end

sk_definition_writer.add_definition('switches', 'struct SwitchDefinition[]', '_geo', switches)

local ball_launchers = {}

for _, switch_element in pairs(sk_scene.nodes_for_type('@ball_launcher')) do
    local position, rotation = switch_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(switch_element.node)

    table.insert(ball_launchers, {
        position = position,
        rotation = rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        roomIndex = room_index,
        signalIndex = signals.signal_index_for_name(switch_element.arguments[1]),
        ballLifetime = switch_element.arguments[2] and tonumber(switch_element.arguments[2]) or 10,
        ballVelocity = switch_element.arguments[3] and tonumber(switch_element.arguments[3]) or 3
    })
end

sk_definition_writer.add_definition('ball_launchers', 'struct BallLauncherDefinition[]', '_geo', ball_launchers)

local ball_catchers = {}

for _, switch_element in pairs(sk_scene.nodes_for_type('@ball_catcher')) do
    local position, rotation = switch_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(switch_element.node)

    table.insert(ball_catchers, {
        position = position,
        rotation = rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        roomIndex = room_index,
        signalIndex = signals.signal_index_for_name(switch_element.arguments[1]),
    })
end

sk_definition_writer.add_definition('ball_catchers', 'struct BallCatcherDefinition[]', '_geo', ball_catchers)

local clocks = {}

for _, clock_element in pairs(sk_scene.nodes_for_type('@clock')) do
    local position, rotation = clock_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(clock_element.node)

    table.insert(clocks, {
        position = position,
        rotation = rotation,
        roomIndex = room_index,
        duration = tonumber(clock_element.arguments[1]),
    })
end

sk_definition_writer.add_definition('clocks', 'struct ClockDefinition[]', '_geo', clocks)

local security_cameras = {}

for _, security_camera_element in pairs(sk_scene.nodes_for_type('@security_camera')) do
    local position, rotation = security_camera_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(security_camera_element.node)

    table.insert(security_cameras, {
        position = position,
        rotation = rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        roomIndex = room_index,
    })
end

sk_definition_writer.add_definition('security_cameras', 'struct SecurityCameraDefinition[]', '_geo', security_cameras)

local turrets = {}

for _, turret_element in pairs(sk_scene.nodes_for_type('@turret')) do
    local position, rotation = turret_element.node.full_transformation:decompose()

    local room_index = room_export.node_nearest_room_index(turret_element.node)
    local player_can_autotip = sk_scene.find_flag_argument(turret_element.arguments, "player_can_autotip")

    table.insert(turrets, {
        position = position,
        rotation = rotation * sk_math.axis_angle(sk_math.vector3(1, 0, 0), math.pi * 0.5),
        roomIndex = room_index,
        playerCanAutotip = player_can_autotip,
    })
end

sk_definition_writer.add_definition('turrets', 'struct TurretDefinition[]', '_geo', turrets)

local function generate_static_collision_boxes()
    local collision_boxes = {}

    for _, fizzler in pairs(fizzlers) do
        local fizzler_box_half_size = sk_math.vector3(0.125, fizzler.height, 0.125)
        local basis = {
            x = fizzler.rotation * sk_math.Vector3.RIGHT,
            y = fizzler.rotation * sk_math.Vector3.UP,
            z = fizzler.rotation * sk_math.Vector3.FORWARD
        }

        table.insert(collision_boxes, {
            basis = basis,
            position = fizzler.position + ((fizzler.width - fizzler_box_half_size.x) * -basis.x),
            half_size = fizzler_box_half_size
        })
        table.insert(collision_boxes, {
            basis = basis,
            position = fizzler.position + ((fizzler.width - fizzler_box_half_size.x) * basis.x),
            half_size = fizzler_box_half_size
        })
    end

    return collision_boxes
end

return {
    entities = {
        box_droppers = box_droppers,
        buttons = buttons,
        decor = decor,
        elevators = elevators,
        fizzlers = fizzlers,
        pedestals = pedestals,
        signage = signage,
        switches = switches,
        ball_catchers = ball_catchers,
        ball_launchers =  ball_launchers,
        clocks = clocks,
        security_cameras = security_cameras,
        turrets = turrets,
    },
    static_collision_boxes = generate_static_collision_boxes(),
}