
Vector3 = {}

function vector3(x, y, z) 
    return setmetatable({ x = x, y = y, z = z }, Vector3)
end

function Vector3.__add(a, b)
    if (type(a) == 'number') then
        return vector3(a + b.x, a + b.y, a + b.z)
    end

    if (type(b) == 'number') then
        return vector3(a.x + b, a.y + b, a.z + b)
    end

    if (type(b) ~= 'table' or type(b.x) ~= 'number' or type(b.y) ~= 'number' or type(b.z) ~= 'number') then
        error('Vector3.__add expected another vector as second operand')
    end

    return vector3(a.x + b.x, a.y + b.y, a.z + b.z)
end

function Vector3.__tostring(v)
    return 'vector3(' .. v.x .. ', ' .. v.y .. ', ' .. v.z .. ')'
end