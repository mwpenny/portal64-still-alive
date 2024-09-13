local util = require('tools.level_scripts.util')
local yaml = require('yaml')
local sk_input = require('sk_input')

local file_stem = sk_input.input_filename:match(".+[\\/]([^\\.]+)")
local file_location = util.path_join(
    "assets",
    "test_chambers",
    file_stem,
    file_stem .. ".yaml"
)

local input_file = io.open(file_location, 'r')
local json_contents = yaml.parse(input_file:read('a'))
input_file:close()

return {
    json_contents = json_contents,
}