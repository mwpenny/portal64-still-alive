#include "LuaGeometry.h"

#include "LuaBasicTypes.h"

void toLua(lua_State* L, const aiVector3D& vector) {
    lua_getglobal(L, "vector3");
    toLua(L, vector.x);
    toLua(L, vector.y);
    toLua(L, vector.z);
    lua_call(L, 3, 1);
}

void toLua(lua_State* L, const aiQuaternion& quaternion) {
    lua_getglobal(L, "quaternion");
    toLua(L, quaternion.x);
    toLua(L, quaternion.y);
    toLua(L, quaternion.z);
    toLua(L, quaternion.w);
    lua_call(L, 4, 1);
}

void toLua(lua_State* L, const aiAABB& box) {
    lua_getglobal(L, "box3");
    toLua(L, box.mMin);
    toLua(L, box.mMax);
    lua_call(L, 2, 1);
}

void fromLua(lua_State* L, aiVector3D& vector) {
    lua_getfield(L, -1, "x");
    fromLua(L, vector.x);

    lua_getfield(L, -1, "y");
    fromLua(L, vector.y);

    lua_getfield(L, -1, "z");
    fromLua(L, vector.z);

    lua_pop(L, 1);
}