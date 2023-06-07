--- @module sk_math

local Vector3 = {}
local Box3 = {}
local Quaternion = {}
local Color4 = {}

--- creates a new 3d vector
--- @function vector3
--- @tparam number x the x value for the vector
--- @tparam number y the x value for the vector
--- @tparam number z the x value for the vector
--- @treturn Vector3
local function vector3(x, y, z) 
    return setmetatable({ x = x or 0, y = y or 0, z = z or 0 }, Vector3)
end

--- determines if the input is a Vector3
--- @function isVector3
--- @tparam any obj
--- @treturn boolean
local function isVector3(obj)
    return type(obj) == 'table' and type(obj.x) == 'number' and type(obj.y) == 'number' and type(obj.z) == 'number' and obj.w == nil
end

--- creates a box3
--- @function box3
--- @tparam Vector3 min
--- @tparam Vector3 max
--- @treturn Box3
local function box3(min, max)
    return setmetatable({ min = min or vector3(), max = max or vector3() }, Box3)
end

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

--- creates a new quaternion with an axis and an angle in radians
--- @function quaternion
--- @tparam Vector3 axis
--- @tparam number angle
--- @treturn Quaternion
local function axis_angle(axis, angle)
    local normalized_axis = axis:normalized()
    local cos_angle = math.cos(angle * 0.5)
    local sin_angle = math.sin(angle * 0.5)

    return quaternion(
        normalized_axis.x * sin_angle, 
        normalized_axis.y * sin_angle,
        normalized_axis.z * sin_angle,
        cos_angle
    )
end

--- determines if the input is a Quaternion
--- @function isQuaternion
--- @tparam any obj
--- @treturn boolean
local function isQuaternion(obj)
    return type(obj) == 'table' and type(obj.x) == 'number' and type(obj.y) == 'number' and type(obj.z) == 'number' and type(obj.w) == 'number'
end

--- @type Vector3
--- @tfield number x
--- @tfield number y
--- @tfield number z
Vector3.__index = Vector3;

--- @function __eq
--- @tparam number|Vector3 b
--- @treturn Vector3
function Vector3.__eq(a, b)
    if (type(a) == 'number') then
        return a == b.x and a == b.y and a == b.z
    end

    if (type(b) == 'number') then
        return a.x == b and a.y == b and a.z + b
    end

    if (not isVector3(b)) then
        error('Vector3.__eq expected another vector as second operand', 2)
    end

    return a.x == b.x and a.y == b.y and a.z == b.z
end

--- @function __add
--- @tparam number|Vector3 b
--- @treturn Vector3
function Vector3.__add(a, b)
    if (type(a) == 'number') then
        return vector3(a + b.x, a + b.y, a + b.z)
    end

    if (type(b) == 'number') then
        return vector3(a.x + b, a.y + b, a.z + b)
    end

    if (not isVector3(b)) then
        error('Vector3.__add expected another vector as second operand got ' .. type(b), 2)
    end

    return vector3(a.x + b.x, a.y + b.y, a.z + b.z)
end

--- @function __sub
--- @tparam number|Vector3 b
--- @treturn Vector3
function Vector3.__sub(a, b)
    if (type(a) == 'number') then
        return vector3(a - b.x, a - b.y, a - b.z)
    end

    if (type(b) == 'number') then
        return vector3(a.x - b, a.y - b, a.z - b)
    end

    if (not isVector3(b)) then
        error('Vector3.__sub expected another vector as second operand', 2)
    end

    if (a == nil) then
        print(debug.traceback())
    end

    return vector3(a.x - b.x, a.y - b.y, a.z - b.z)
end

--- @function __mul
--- @tparam number|Vector3 b
--- @treturn Vector3
function Vector3.__mul(a, b)
    if (type(a) == 'number') then
        return vector3(a * b.x, a * b.y, a * b.z)
    end

    if (type(b) == 'number') then
        return vector3(a.x * b, a.y * b, a.z * b)
    end

    if (not isVector3(b)) then
        error('Vector3.__mul expected another vector or number as second operand got ' .. type(b), 2)
    end

    return vector3(a.x * b.x, a.y * b.y, a.z * b.z)
end

--- @function __div
--- @tparam number|Vector3 b
--- @treturn Vector3
function Vector3.__div(a, b)
    if (type(a) == 'number') then
        return vector3(a / b.x, a / b.y, a / b.z)
    end

    if (type(b) == 'number') then
        local mul_value = 1 / b
        
        return vector3(a.x * mul_value, a.y * mul_value, a.z * mul_value)
    end

    if (not isVector3(b)) then
        error('Vector3.__div expected another vector as second operand', 2)
    end

    return vector3(a.x / b.x, a.y / b.y, a.z / b.z)
end

function Vector3.__tostring(v)
    return 'vector3(' .. v.x .. ', ' .. v.y .. ', ' .. v.z .. ')'
end

--- Get the magnitude of the vector
--- @function magnitude
--- @treturn number
function Vector3.magnitude(v)
    return math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
end

--- Get the magnitude squared of the vector
--- @function magnitudeSqrd
--- @treturn number
function Vector3.magnitudeSqrd(v)
    return v.x * v.x + v.y * v.y + v.z * v.z
end

--- Returns a normalized version of this vector
--- @function normalized
--- @treturn Vector3
function Vector3.normalized(v)
    local magnitude = v:magnitude()

    if (magnitude == 0) then
        return vector3(0, 0, 0)
    end

    return v / magnitude
end

--- Get the magnitude of the vector
--- @function min
--- @tparam Vector3 other vector
--- @treturn Vector3
function Vector3.min(a, b)
    return vector3(math.min(a.x, b.x), math.min(a.y, b.y), math.min(a.z, b.z))
end

--- Get the magnitude of the vector
--- @function max
--- @tparam Vector3 other vector
--- @treturn Vector3
function Vector3.max(a, b)
    return vector3(math.max(a.x, b.x), math.max(a.y, b.y), math.max(a.z, b.z))
end

--- Get the dot product between two vectors
--- @function dot
--- @tparam Vector3 b
--- @treturn number
function Vector3.dot(a, b)
    return a.x * b.x + a.y * b.y + a.z * b.z
end

--- Get the cross product between two vectors
--- @function cross
--- @tparam Vector3 b
--- @treturn Vector3
function Vector3.cross(a, b)
    return vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    )
end

--- Linearly interpolates between two points
--- @function lerp
--- @tparam Vector3 b
--- @treturn Vector3
function Vector3.lerp(a, b, lerp)
    return a * (1 - lerp) + b * lerp
end

--- @type Box3
--- @tfield Vector3 min
--- @tfield Vector3 max
Box3.__index = Box3;

--- Returns the point inside or on the box that is nearest to the given point
--- @function nearest_point_in_box
--- @tparam Vector3 point
--- @treturn Vector3
function Box3.nearest_point_in_box(box, point)
    return Vector3.min(box.max, point):max(box.min)
end

--- Returns the point inside or on the box that is nearest to the given point
--- @function overlaps
--- @tparam Vector3|Box3 box_or_point
--- @treturn boolean
function Box3.overlaps(box, box_or_point)
    if isVector3(box_or_point) then
        return box_or_point.x >= box.min.x and box_or_point.x <= box.max.x and
            box_or_point.y >= box.min.y and box_or_point.y <= box.max.y and
            box_or_point.z >= box.min.z and box_or_point.z <= box.max.z
    end

    return box.min.x < box_or_point.max.x and box_or_point.min.x < box.max.x and
        box.min.y < box_or_point.max.y and box_or_point.min.y < box.max.y and
        box.min.z < box_or_point.max.z and box_or_point.min.z < box.max.z;
end

--- @function __mul
--- @tparam number|Box3 b
--- @treturn Box3
function Box3.__mul(a, b)
    if type(a) == 'number' then
        return box3(a * b.min, a * b.max)
    end

    if type(b) == 'number' then
        return box3(a.min * b, a.max * b)
    end

    return box3(a.min * b.min, a.max * b.max)
end

--- Gets the distance from the box to the point
--- If the box contains the point then the negative distance to
--- the nearest edge is returned
--- @function distance_to_point
--- @tparam Vector3 point
--- @treturn number
function Box3.distance_to_point(box, point)
    local nearest_point = Box3.nearest_point_in_box(box, point)

    if (nearest_point == point) then
        local max_offset = Vector3.__sub(point, box.max)
        local min_offset = Vector3.__sub(box.min, point)

        return math.max(
            max_offset.x, max_offset.y, max_offset.z, 
            min_offset.x, min_offset.y, min_offset.z
        )
    end

    return (nearest_point - point):magnitude()
end


--- Linearly interpolates between the min and max of the box
--- @function lerp
--- @treturn Vector3
function Box3.lerp(box, lerp)
    return Vector3.lerp(box.min, box.max, lerp)
end

--- Finds a lerp value, x, such that box:lerp(x) == pos
--- @function pos
--- @treturn Vector3
function Box3.unlerp(box, pos)
    return (pos - box.min) / (box.max - box.min)
end


--- Linearly interpolates between the min and max of the box
--- @function union
--- @tparam Vector3|Box3 box_or_point
--- @treturn Box3
function Box3.union(box, box_or_point)
    if isVector3(box_or_point) then
        return box3(box.min:min(box_or_point), box.max:max(box_or_point))
    end

    return box3(box.min:min(box_or_point.min), box.max:max(box_or_point.max))
end

function Box3.__tostring(b)
    return 'box3(' .. tostring(b.min) .. ', ' .. tostring(b.max) .. ')'
end

--- @type Quaternion
--- @tfield number x
--- @tfield number y
--- @tfield number z
--- @tfield number w
Quaternion.__index = Quaternion;

--- @function conjugate
--- @treturn Quaternion
function Quaternion.conjugate(input)
    return quaternion(-input.x, -input.y, -input.z, input.w)
end

function Quaternion.__tostring(v)
    return 'quaternion(' .. v.x .. ', ' .. v.y .. ', ' .. v.z .. ', ' .. v.w .. ')'
end

function Quaternion.__mul(a, b)
    if (isVector3(b)) then
        local result = a * quaternion(b.x, b.y, b.z, 0) * a:conjugate()
        return vector3(result.x, result.y, result.z)
    elseif (isQuaternion(b)) then
        return quaternion(
            a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
            a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z,
            a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x,
            a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
        )
    else
        error("Expected vector3 or quaternion got " .. tostring(b), 2)
    end
end

--- @function slerp
--- @tparam Quaternion b
--- @tparam number t
--- @treturn Quaternion
function Quaternion.slerp(a, b, t)
    -- calc cosine theta
    local cosom = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    -- adjust signs (if necessary)
    local endQ = quaternion(b.x, b.y, b.z, b.w);

    if cosom < 0 then
        cosom = -cosom
        -- Reverse all signs
        endQ.x = -endQ.x   
        endQ.y = -endQ.y
        endQ.z = -endQ.z
        endQ.w = -endQ.w
    end

    -- Calculate coefficients
    local sclp, sclq;
    if (1 - cosom) > 0.0001 then
        -- Standard case (slerp)
        local omega = math.acos(cosom); -- extract theta from dot product's cos theta
        local sinom = math.sin( omega);
        sclp  = math.sin( (1 - t) * omega) / sinom;
        sclq  = math.sin( t * omega) / sinom;
    else
        -- Very close, do linear interp (because it's faster)
        sclp = 1 - t;
        sclq = t;
    end

    return quaternion(
        sclp * a.x + sclq * endQ.x,
        sclp * a.y + sclq * endQ.y,
        sclp * a.z + sclq * endQ.z,
        sclp * a.w + sclq * endQ.w
    )
end

--- creates a new 4d color
--- @function color
--- @tparam number r
--- @tparam number g
--- @tparam number b
--- @tparam number a
--- @treturn Color4
local function color4(r, g, b, a) 
    return setmetatable({ r = r or 1, g = g or 1, b = b or 1, a = a or 1 }, Color4)
end

--- determines if the input is a Vector3
--- @function isColor4
--- @tparam any obj
--- @treturn boolean
local function isColor4(obj)
    return type(obj) == 'table' and type(obj.r) == 'number' and type(obj.g) == 'number' and type(obj.b) == 'number' and type(obj.a) == 'number'
end

--- @type Color4
--- @tfield number r
--- @tfield number g
--- @tfield number b
--- @tfield number a
Color4.__index = Color4;


--- @function __eq
--- @tparam number|Color4 b
--- @treturn Color4
function Color4.__eq(a, b)
    if (type(a) == 'number') then
        return a == b.r and a == b.g and a == b.b and a == b.a
    end

    if (type(b) == 'number') then
        return a.r == b and a.g == b and a.b + b and a.a == b
    end

    if (not isColor4(b)) then
        error('Color4.__eq expected another vector as second operand', 2)
    end

    return a.r == b.r and a.g == b.g and a.b == b.b and a.a == a.a
end

--- @function __add
--- @tparam number|Color4 b
--- @treturn Color4
function Color4.__add(a, b)
    if (type(a) == 'number') then
        return color4(a + b.r, a + b.g, a + b.b, a + b.a)
    end

    if (type(b) == 'number') then
        return color4(a.r + b, a.g + b, a.b + b, a.a + b)
    end

    if (not isColor4(b)) then
        error('Color4.__add expected another vector as second operand got ' .. type(b), 2)
    end

    return color4(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a)
end

--- @function __sub
--- @tparam number|Color4 b
--- @treturn Color4
function Color4.__sub(a, b)
    if (type(a) == 'number') then
        return color4(a - b.r, a - b.g, a - b.b, a - b.a)
    end

    if (type(b) == 'number') then
        return color4(a.r - b, a.g - b, a.b - b, a.a - b)
    end

    if (not isColor4(b)) then
        error('Color4.__sub expected another vector as second operand', 2)
    end

    if (a == nil) then
        print(debug.traceback())
    end

    return color4(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a)
end

--- @function __mul
--- @tparam number|Color4 b
--- @treturn Color4
function Color4.__mul(a, b)
    if (type(a) == 'number') then
        return color4(a * b.r, a * b.g, a * b.b, a * b.a)
    end

    if (type(b) == 'number') then
        return color4(a.r * b, a.g * b, a.b * b, a.a * b)
    end

    if (not isColor4(b)) then
        error('Color4.__mul expected another vector or number as second operand got ' .. type(b), 2)
    end

    return color4(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a)
end

function Color4.__tostring(v)
    return 'color4(' .. v.r .. ', ' .. v.g .. ', ' .. v.b .. ', ' .. v.a .. ')'
end

--- Linearly interpolates between two points
--- @function lerp
--- @tparam Color4 b
--- @treturn Color4
function Color4.lerp(a, b, lerp)
    return a * (1 - lerp) + b * lerp
end

return {
    vector3 = vector3,
    Vector3 = Vector3,
    isVector3 = isVector3,
    box3 = box3,
    Box3 = Box3,
    Quaternion = Quaternion,
    quaternion = quaternion,
    axis_angle = axis_angle,
    isQuaternion = isQuaternion,
    color4 = color4,
    isColor4 = isColor4,
}