#include "LuaGeometry.h"

#include "LuaBasicTypes.h"
#include "LuaUtils.h"

void toLua(lua_State* L, const aiVector3D& vector) {
    luaLoadModuleFunction(L, "sk_math", "vector3");
    toLua(L, vector.x);
    toLua(L, vector.y);
    toLua(L, vector.z);
    lua_call(L, 3, 1);
}

void toLua(lua_State* L, const aiColor4D& color) {
    luaLoadModuleFunction(L, "sk_math", "color4");
    toLua(L, color.r);
    toLua(L, color.g);
    toLua(L, color.b);
    toLua(L, color.a);
    lua_call(L, 4, 1);
    
}

void toLua(lua_State* L, const aiQuaternion& quaternion) {
    luaLoadModuleFunction(L, "sk_math", "quaternion");
    toLua(L, quaternion.x);
    toLua(L, quaternion.y);
    toLua(L, quaternion.z);
    toLua(L, quaternion.w);
    lua_call(L, 4, 1);
}

void toLua(lua_State* L, const aiAABB& box) {
    luaLoadModuleFunction(L, "sk_math", "box3");
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

void fromLua(lua_State* L, aiQuaternion& quaternion) {
    lua_getfield(L, -1, "x");
    fromLua(L, quaternion.x);

    lua_getfield(L, -1, "y");
    fromLua(L, quaternion.y);

    lua_getfield(L, -1, "z");
    fromLua(L, quaternion.z);

    lua_getfield(L, -1, "w");
    fromLua(L, quaternion.w);

    lua_pop(L, 1);
}

void fromLua(lua_State* L, aiColor4D& color) {
    lua_getfield(L, -1, "r");
    fromLua(L, color.r);

    lua_getfield(L, -1, "g");
    fromLua(L, color.g);

    lua_getfield(L, -1, "b");
    fromLua(L, color.b);

    lua_getfield(L, -1, "a");
    fromLua(L, color.a);

    lua_pop(L, 1);
}