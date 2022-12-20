
local sk_definition_writer = require('sk_definition_writer')
local static_export = require('tools.level_scripts.static_export')
local collision_export = require('tools.level_scripts.collision_export')
local sk_math = require('sk_math')

sk_definition_writer.add_definition("level", "struct LevelDefinition", "_geo", {
    collisionQuads = sk_definition_writer.reference_to(collision_export.collision_objects, 1),
    staticContent = sk_definition_writer.reference_to(static_export.static_content_elements, 1),
    roomStaticMapping = sk_definition_writer.reference_to(static_export.room_ranges, 1),
})