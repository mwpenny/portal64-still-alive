/// @module sk_input

#include "LuaDisplayListSettings.h"

#include <string.h>
#include "./LuaMesh.h"
#include "LuaTransform.h"
#include "LuaBasicTypes.h"
#include "LuaUtils.h"

Material* luaGetMaterial(lua_State* L, const DisplayListSettings& defaults) {
    int materialType = lua_type(L, -1);

    if (materialType == LUA_TSTRING) {
        auto material = defaults.mMaterials.find(lua_tostring(L, -1));

        if (material != defaults.mMaterials.end()) {
            return material->second.get();
        }
    }

    if (materialType == LUA_TTABLE) {
        lua_pushnil(L);
        lua_copy(L, -2, -1);
        Material* result = nullptr;
        fromLua(L, result);
        return result;
    }

    return nullptr;
}

void fromLua(lua_State* L, DisplayListSettings& result, const DisplayListSettings& defaults) {
    result = defaults;

    int topStart = lua_gettop(L);

    lua_pushnil(L);  /* first key */
    while (lua_next(L, topStart) != 0) {
        int keyType = lua_type(L, -2);
        
        if (keyType == LUA_TSTRING) {
            const char* key = lua_tostring(L, -2);

            if (strcmp(key, "defaultMaterial") == 0) {
                Material* material = luaGetMaterial(L, defaults);

                if (!material) {
                    luaL_error(L, "invlaid defaultMaterial");
                }

                result.mDefaultMaterialState = material->mState;
            }
        }

        lua_pop(L, 1);
    }

}

/***
 @table settings
 @tfield sk_transform.Transform model_transform
 @tfield sk_transform.Transform fixed_point_transform
 @tfield number model_scale
 @tfield number fixed_point_scale
 */

/***
 @table input_filename
 @tfield foo bar
 */

int luaInputModuleLoader(lua_State* L) {
    lua_newtable(L);

    DisplayListSettings* defaults = (DisplayListSettings*)lua_touserdata(L, lua_upvalueindex(1));
    const char* levelFilename = lua_tostring(L, lua_upvalueindex(2));

    lua_newtable(L);
    toLua(L, defaults->CreateCollisionTransform());
    lua_setfield(L, -2, "model_transform");

    toLua(L, defaults->CreateGlobalTransform());
    lua_setfield(L, -2, "fixed_point_transform");

    toLua(L, defaults->mModelScale);
    lua_setfield(L, -2, "model_scale");

    toLua(L, defaults->mFixedPointScale);
    lua_setfield(L, -2, "fixed_point_scale");

    toLua(L, defaults->mTicksPerSecond);
    lua_setfield(L, -2, "ticks_per_second");

    lua_setfield(L, -2, "settings");

    lua_pushstring(L, levelFilename);
    lua_setfield(L, -2, "input_filename");

    return 1;
}

void populateDisplayListSettings(lua_State* L, const DisplayListSettings& defaults, const std::string& levelFilename) {
    lua_pushlightuserdata(L, const_cast<DisplayListSettings*>(&defaults));
    lua_pushstring(L, levelFilename.c_str());
    lua_pushcclosure(L, luaInputModuleLoader, 2);
    luaSetModuleLoader(L, "sk_input");
}