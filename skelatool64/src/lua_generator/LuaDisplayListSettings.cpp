#include "LuaDisplayListSettings.h"

#include <string.h>
#include "./LuaMesh.h"

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