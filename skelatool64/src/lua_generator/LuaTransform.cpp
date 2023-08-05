/// @module sk_transform

#include "LuaTransform.h"

#include "LuaBasicTypes.h"
#include "LuaUtils.h"

#include <sstream>
#include <iomanip>

/***
 @function from_pos_rot_scale
 @tparam sk_math.Vector3 pos
 @tparam[opt] sk_math.Quaternion rot
 @tparam[opt] sk_math.Scale scale
 @treturn Transform
 */
int luaTransformFromPosRotationScale(lua_State* L) {
    if (lua_gettop(L) > 3) {
        lua_settop(L, 3);
    }

    aiVector3D scale;

    if (lua_gettop(L) == 3) {
        fromLua(L, scale);
    } else {
        scale = aiVector3D(1, 1, 1);
    }

    aiQuaternion rot;

    if (lua_gettop(L) == 2) {
        fromLua(L, rot);
    }

    aiVector3D pos;

    fromLua(L, pos);

    aiMatrix4x4 fullTransform(scale, rot, pos);
    toLua(L, fullTransform);
    return 1;
}

/***
 @function from_pos_rot_scale
 @tparam {...number} data
 @treturn Transform
 */
int luaTransformFromArray(lua_State* L) {
    lua_type(L, 1);

    if (lua_isnil(L, 1)) {
        return 0;
    }

    double value[16];

    for (int i = 0; i < 16; ++i) {
        if (lua_geti(L, 1, i + 1) == LUA_TNUMBER) {
            value[i] = lua_tonumber(L, -1);
        } else {
            value[i] = 0.0;
        }
        lua_pop(L, 1);
    }

    aiMatrix4x4 fullTransform(
        value[0], value[1], value[2], value[3],
        value[4], value[5], value[6], value[7],
        value[8], value[9], value[10], value[11],
        value[12], value[13], value[14], value[15]
    );
    toLua(L, fullTransform);
    return 1;
}

/***
A 4x4 matrix transform
@type Transform
*/

void toLua(lua_State* L, const aiMatrix4x4& matrix) {
    aiMatrix4x4* result = (aiMatrix4x4*)lua_newuserdata(L, sizeof(aiMatrix4x4));
    new(result) aiMatrix4x4(matrix);

    luaL_getmetatable(L, "aiMatrix4x4");
    lua_setmetatable(L, -2);
}


void fromLua(lua_State* L, aiMatrix4x4& matrix) {
    aiMatrix4x4* ptr = (aiMatrix4x4*)luaL_checkudata(L, -1, "aiMatrix4x4");
    if (ptr) {
        matrix = *ptr;
    }
    lua_pop(L, 1);
}

/***
@function decompose
@treturn vector3.Vector3 position
@treturn quaternion.Quaternion rotation
@treturn vector3.Vector3 scale
 */
int luaTransformDecompose(lua_State* L) {
    aiMatrix4x4* mtx = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D position;
    mtx->Decompose(scaling, rotation, position);

    toLua(L, position);
    toLua(L, rotation);
    toLua(L, scaling);

    return 3;
}

/***
@function inverse
@treturn Transform
 */
int luaTransformInverse(lua_State* L) {
    aiMatrix4x4 mtx = *(aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");
    mtx.Inverse();
    toLua(L, mtx);
    return 1;
}

int luaTransformIndex(lua_State* L) {
    aiMatrix4x4* mtx = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    int keyType = lua_type(L, 2);

    if (keyType == LUA_TSTRING) {
        const char* key = lua_tostring(L, 2);

        if (strcmp(key, "decompose") == 0) {
            lua_pushcfunction(L, luaTransformDecompose);
        } else if (strcmp(key, "inverse") == 0) {
            lua_pushcfunction(L, luaTransformInverse);
        } else {
            lua_pushnil(L);
        }

        return 1;
    } else if (keyType == LUA_TTABLE) {
        lua_len(L, 2);
        int length = lua_tointeger(L, -1);
        lua_pop(L, 1);

        if (length == 2) {
            lua_geti(L, 2, 1);
            lua_geti(L, 2, 2);

            int row = lua_tointeger(L, -2);
            int col = lua_tointeger(L, -1);

            lua_pushnumber(L, (*mtx)[row - 1][col - 1]);
            return 1;
        }
    }

    lua_pushfstring(L, "Expected a string key or an array of length 2 got '%s'", lua_typename(L, keyType));
    lua_error(L);
    return 0;
}

bool luaIsTransform(lua_State* L, int idx) {
    if (lua_type(L, idx) != LUA_TUSERDATA) {
        return false;
    }
    
    if (!lua_getmetatable(L, idx)) {
        return false;
    }

    luaL_getmetatable(L, "aiMatrix4x4");

    bool result = lua_rawequal(L, -2, -1);
    lua_pop(L, 2);
    return result;
}

/**
 @function __mul
 @tparam Transform|sk_math.Vector3 other
 @treturn Transform
 */
int luaTransformMul(lua_State* L) {
    aiMatrix4x4* a = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    if (luaIsTransform(L, 2)) {
        aiMatrix4x4* b = (aiMatrix4x4*)luaL_checkudata(L, 2, "aiMatrix4x4");

        aiMatrix4x4 result = (*a) * (*b);

        toLua(L, result);

        return 1;
    }

    if (!lua_istable(L, -1)) {
        luaL_checkudata(L, 2, "aiMatrix4x4");
        return 0;
    }

    aiVector3D asPoint;
    fromLua(L, asPoint);

    toLua(L, (*a) * asPoint);

    return 1;
}

int luaTransformToString(lua_State* L) {
    aiMatrix4x4* a = (aiMatrix4x4*)luaL_checkudata(L, 1, "aiMatrix4x4");

    std::ostringstream result;

    result << "| " << std::setprecision(5) << std::fixed << std::setw(5) << a->a1 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->a2 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->a3 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->a4 << " |" << std::endl;

    result << "| " << std::setprecision(5) << std::fixed << std::setw(5) << a->b1 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->b2 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->b3 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->b4 << " |" << std::endl;

    result << "| " << std::setprecision(5) << std::fixed << std::setw(5) << a->c1 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->c2 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->c3 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->c4 << " |" << std::endl;

    result << "| " << std::setprecision(5) << std::fixed << std::setw(5) << a->d1 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->d2 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->d3 << " ";
    result << std::setprecision(5) << std::fixed << std::setw(5) << a->d4 << " |" << std::endl;

    toLua(L, result.str());
    return 1;
}

int buildTransformModule(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, luaTransformFromPosRotationScale);
    lua_setfield(L, -2, "from_pos_rot_scale");

    lua_pushcfunction(L, luaTransformFromArray);
    lua_setfield(L, -2, "from_array");

    return 1;
}

void generateLuaTransform(lua_State* L) {
    luaL_newmetatable(L, "aiMatrix4x4");

    lua_pushcfunction(L, luaTransformIndex);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, luaTransformMul);
    lua_setfield(L, -2, "__mul");

    lua_pushcfunction(L, luaTransformToString);
    lua_setfield(L, -2, "__tostring");

    lua_pop(L, 1);

    lua_pushcfunction(L, buildTransformModule);
    luaSetModuleLoader(L, "sk_transform");
}