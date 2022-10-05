
Quaternion = {}

function quaternion(x, y, z, w) 
    return setmetatable({ x = x, y = y, z = z, w = w }, Quaternion)
end

function quaternionConjugate(input)
    return quaternion(-input.x, -input.y, -input.z, input.w)
end

function Quaternion.__tostring(v)
    return 'quaternion(' .. v.x .. ', ' .. v.y .. ', ' .. v.z .. ', ' .. v.w .. ')'
end

function Quaternion.__mul(a, b)
    if (isVector3(b)) then
        local result = a * quaternion(b.x, b.y, b.z, 0) * quaternionConjugate(a)
        return vector3(result.x, result.y, result.z)
    elseif (isQuaternion(b)) then
        return quaternion(
            a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
            a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z,
            a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x,
            a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
        )
    else
        error("Expected vector3 or quaternion got " .. tostring(b))
    end
end

function isQuaternion(obj)
    return type(obj) == 'table' and type(obj.x) == 'number' and type(obj.y) == 'number' and type(obj.z) == 'number' and type(obj.w) == 'number'
end