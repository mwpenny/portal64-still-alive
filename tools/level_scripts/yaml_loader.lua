local yaml = require('yaml')
local sk_input = require('sk_input')

local file_location = string.sub(sk_input.input_filename, 7, -4) .. 'yaml'

local input_file = io.open(file_location, 'r')
local json_contents = yaml.parse(input_file:read('a'))
input_file:close()

local function dump_json(value)
    local file_contents = yaml.stringify(value)
    print(file_contents)
    print(file_location)

    local file = io.open(file_location, 'w');
    file:write(file_contents)
    file:close()
end

return {
    dump_json = dump_json,
    json_contents = json_contents,
}