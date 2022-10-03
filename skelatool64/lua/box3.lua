
Box3 = {}

function box3(min, max)
    return setmetatable({ min = min, max = max }, Box3)
end