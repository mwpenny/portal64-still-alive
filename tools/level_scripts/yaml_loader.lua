local yaml = require('yaml')
local sk_input = require('sk_input')

local file_location = string.sub(sk_input.input_filename, 7, -4) .. 'yaml'

local input_file = io.open(file_location, 'r')
local json_contents = yaml.parse(input_file:read('a'))
input_file:close()

return {
    json_contents = json_contents,
}