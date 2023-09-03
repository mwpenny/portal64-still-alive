
local function string_split(inputstr, sep)
    if sep == nil then
            sep = "%s"
    end
    local t={}
    for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
            table.insert(t, str)
    end
    return t
end

local function trim(inputstr)
    local start_index = 1
    local end_index = #inputstr

    while start_index <= #inputstr and string.sub(inputstr, start_index, start_index) == ' ' do
        start_index = start_index + 1
    end

    while end_index >= 1 and string.sub(inputstr, end_index, end_index) == ' ' do
        end_index = end_index - 1
    end

    if end_index < start_index then
        return ''
    end

    return string.sub(inputstr, start_index, end_index)
end

return {
    string_split = string_split,
    trim = trim,
}