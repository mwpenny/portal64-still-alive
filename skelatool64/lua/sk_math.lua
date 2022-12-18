--- @module sk_math

local exports = {}

--- @table Vector3
--- @tfield number x
--- @tfield number y
--- @tfield number z
local Vector3 = {}

exports.Vector3 = exports

--- creates a new 3d vector
--- @function vector3
--- @tparam number x the x value for the vector
--- @tparam number y the x value for the vector
--- @tparam number z the x value for the vector
--- @treturn Vector3
local function vector3(x, y, z) 
    return setmetatable({ x = x, y = y, z = z }, Vector3)
end

exports.vector3 = vector3

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

local function isVector3(obj)
    return type(obj) == 'table' and type(obj.x) == 'number' and type(obj.y) == 'number' and type(obj.z) == 'number' and obj.w == nil
end

exports.isVector3 = isVector3

--- @table Box3
--- @tfield Vector3 min
--- @tfield Vector3 max
local Box3 = {}

exports.Box3 = Box3

--- creates a box3
--- @function box3
--- @tparam Vector3 min
--- @tparam Vector3 max
--- @treturn Box3
local function box3(min, max)
    return setmetatable({ min = min, max = max }, Box3)
end

exports.box3 = box3

--- @table Quaternion
--- @tfield number x
--- @tfield number y
--- @tfield number z
--- @tfield number w
local Quaternion = {}

exports.Quaternion = Quaternion

--- creates a new quaternion
--- @function quaternion
--- @tparam number x the x value for the quaternion
--- @tparam number y the x value for the quaternion
--- @tparam number z the x value for the quaternion
--- @tparam number w the x value for the quaternion
--- @treturn Quaternion
local function quaternion(x, y, z, w) 
    return setmetatable({ x = x, y = y, z = z, w = w }, Quaternion)
end

exports.quaternion = quaternion

local function quaternionConjugate(input)
    return quaternion(-input.x, -input.y, -input.z, input.w)
end

exports.quaternionConjugate = quaternionConjugate

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

local function isQuaternion(obj)
    return type(obj) == 'table' and type(obj.x) == 'number' and type(obj.y) == 'number' and type(obj.z) == 'number' and type(obj.w) == 'number'
end

exports.isQuaternion = isQuaternion

return exports