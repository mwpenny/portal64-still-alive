
Quaternion = {}

function quaternion(x, y, z, w) 
    return setmetatable({ x = x, y = y, z = z, w = w }, Quaternion)
end

function Quaternion.__tostring(v)
    return 'quaternion(' .. v.x .. ', ' .. v.y .. ', ' .. v.z .. ', ' .. v.w .. ')'
end