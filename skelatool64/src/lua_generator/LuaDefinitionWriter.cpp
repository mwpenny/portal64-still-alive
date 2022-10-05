#include "LuaDefinitionWriter.h"

#include "LuaGenerator.h"
#include "../StringUtils.h"
#include "LuaMesh.h"

std::unique_ptr<DataChunk> buildDataChunk(lua_State* L);

std::unique_ptr<DataChunk> buildMacroChunk(lua_State* L) {
    lua_getfield(L, -1, "name");
    std::string name = lua_tostring(L, -1);
    lua_pop(L, 1);

    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk(name));

    lua_getfield(L, -1, "args");

    int args = lua_gettop(L);
    
    lua_pushnil(L);  /* first key */
    while (lua_next(L, args) != 0) {
        result->Add(std::move(buildDataChunk(L)));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return result;
}

std::unique_ptr<DataChunk> buildStructureChunk(lua_State* L) {
    int topStart = lua_gettop(L);
    
    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    std::vector<std::pair<std::string, std::unique_ptr<DataChunk>>> namedEntries;
    
    lua_pushnil(L);  /* first key */
    while (lua_next(L, topStart) != 0) {
        int keyType = lua_type(L, -2);
        if (keyType == LUA_TNUMBER) {
            result->Add(std::move(buildDataChunk(L)));
        } else if (keyType == LUA_TSTRING) {
            namedEntries.push_back(std::pair<std::string, std::unique_ptr<DataChunk>>(
                lua_tostring(L, -2),
                std::move(buildDataChunk(L))
            ));
        }
        lua_pop(L, 1);
    }

    std::sort(namedEntries.begin(), namedEntries.end());

    for (auto& entry : namedEntries) {
        result->Add(entry.first, std::move(entry.second));
    }

    return result;
}

std::unique_ptr<DataChunk> buildDataChunk(lua_State* L) {
    int type = lua_type(L, -1);

    if (type == LUA_TNUMBER) {
        if (lua_isinteger(L, -1))
        {
            int result = lua_tointeger(L, -1);
            return std::unique_ptr<DataChunk>(new PrimitiveDataChunk<int>(result));
        }

        double result = lua_tonumber(L, -1);
        return std::unique_ptr<DataChunk>(new PrimitiveDataChunk<double>(result));
    }

    if (type == LUA_TNIL) {
        return std::unique_ptr<DataChunk>(new PrimitiveDataChunk<int>(0));
    }

    if (type == LUA_TBOOLEAN) {
        return std::unique_ptr<DataChunk>(new PrimitiveDataChunk<int>(lua_toboolean(L, -1)));
    }

    if (type == LUA_TSTRING) {
        size_t len;
        const char* buffer = lua_tolstring(L, -1, &len);
        std::string bufferAsString(buffer, len);
        return std::unique_ptr<DataChunk>(new StringDataChunk(bufferAsString));
    }

    if (type == LUA_TTABLE) {
        lua_getglobal(L, "is_raw");
        lua_pushnil(L);
        lua_copy(L, -3, -1);
        lua_call(L, 1, 1);
        if (lua_toboolean(L, -1)) {
            // pop the bool
            lua_pop(L, 1);
            lua_getfield(L, -1, "value");
            std::unique_ptr<DataChunk> result(new PrimitiveDataChunk<std::string>(lua_tostring(L, -1)));
            lua_pop(L, 1);
            return result;
        }
        lua_pop(L, 1);

        lua_getglobal(L, "is_macro");
        lua_pushnil(L);
        lua_copy(L, -3, -1);
        lua_call(L, 1, 1);
        if (lua_toboolean(L, -1)) {
            // pop the bool
            lua_pop(L, 1);
            return buildMacroChunk(L);
        }
        lua_pop(L, 1);

        return buildStructureChunk(L);
    }

    if (type == LUA_TUSERDATA) {
        if (luaIsLazyVector3DArray(L, -1)) {
            struct aiVector3DArray* array = (struct aiVector3DArray*)lua_touserdata(L, -1);

            std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

            for (int i = 0; i < array->length; ++i) {
                result->Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(array->vertices[i])));
            }

            return result;
        }
    }

    return std::unique_ptr<DataChunk>(nullptr);
}

void processCFileDefinition(lua_State* L, CFileDefinition& fileDef, const char* filename) {
    int topStart = lua_gettop(L);

    lua_getfield(L, topStart, "name");
    std::string definitionName = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, topStart, "dataType");
    std::string dataType = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, topStart, "location");
    std::string location = lua_tostring(L, -1);
    lua_pop(L, 1);

    bool isArray = false;

    if (EndsWith(dataType, "[]")) {
        isArray = true;
        dataType = dataType.substr(0, dataType.length() - 2);
    }

    lua_getfield(L, topStart, "data");
    std::unique_ptr<DataChunk> dataChunk = buildDataChunk(L);
    lua_pop(L, 1);

    std::unique_ptr<FileDefinition> definition(new DataFileDefinition(dataType, definitionName, isArray, location, std::move(dataChunk)));
    fileDef.AddDefinition(std::move(definition));
}

int luaAddHeader(lua_State* L) {
    CFileDefinition* fileDefinition = (CFileDefinition*)lua_touserdata(L, lua_upvalueindex(1));
    const char* header = luaL_checkstring(L, 1);
    fileDefinition->AddHeader(header);
    return 0;
}

bool dumpDefinitions(lua_State* L, CFileDefinition& fileDef, const char* filename) {
    int topStart = lua_gettop(L);

    lua_getglobal(L, "consume_pending_definitions");
    int errcode = lua_pcall(L, 0, 1, 0);

    if (checkLuaError(L, errcode, filename)) {
        lua_settop(L, topStart);
        return false;
    }

    int definitionArray = lua_gettop(L);

    lua_pushnil(L);  /* first key */
    while (lua_next(L, definitionArray) != 0) {
        int entry = lua_gettop(L);

        lua_getfield(L, entry, "nameHint");
        const char* nameHint = lua_tostring(L, -1);
        std::string name = fileDef.GetUniqueName(nameHint);
        lua_pop(L, 1);

        lua_pushstring(L, name.c_str());
        lua_setfield(L, entry, "name");

        lua_pop(L, 1);
    }

    lua_getglobal(L, "process_definitions");
    lua_pushnil(L);

    lua_copy(L, definitionArray, -1);

    errcode = lua_pcall(L, 1, LUA_MULTRET, 0);

    if (checkLuaError(L, errcode, filename)) {
        lua_settop(L, topStart);
        return false;
    }

    lua_settop(L, definitionArray);

    lua_pushnil(L);  /* first key */
    while (lua_next(L, topStart + 1) != 0) {
        processCFileDefinition(L, fileDef, filename);
        lua_pop(L, 1);
    }

    lua_settop(L, topStart);

    return true;
}

void populateLuaDefinitionWrite(lua_State* L, CFileDefinition& fileDef) {
    lua_pushlightuserdata(L, &fileDef);
    lua_pushcclosure(L, luaAddHeader, 1);
    lua_setglobal(L, "add_header");
}