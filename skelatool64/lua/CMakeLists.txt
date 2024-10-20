set(LUA_SCRIPTS
    sk_animation.lua
    sk_definition_writer.lua
    sk_math.lua
    sk_scene.lua
)

function(_add_compile_lua_command INPUT_FILE OUTPUT_LIST)
    cmake_path(
        RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR
        BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE RELATIVE_CURRENT_DIR
    )
    cmake_path(
        REMOVE_EXTENSION INPUT_FILE
        OUTPUT_VARIABLE INPUT_NAME
    )

    set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
    set(OUTPUT_FILE_LUAC "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${INPUT_NAME}.out")
    set(OUTPUT_FILE_OBJ "${OUTPUT_FILE_LUAC}${CMAKE_CXX_OUTPUT_EXTENSION}")

    add_custom_command(
        DEPENDS
            ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE_LUAC} ${OUTPUT_FILE_OBJ}
        COMMAND
            # TODO: find dynamically
            luac -o ${OUTPUT_FILE_LUAC} ${INPUT_FILE}
        COMMAND
            # Not cross-platform :(
            # TODO: don't compile these. Read them at runtime.
            ${CMAKE_LINKER} -r -b binary -o ${OUTPUT_FILE_OBJ}
            "$<PATH:RELATIVE_PATH,${OUTPUT_FILE_LUAC},${PROJECT_SOURCE_DIR}>"
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Compiling $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE_OBJ})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

set(LUA_OBJS "")

foreach(LUA_SCRIPT ${LUA_SCRIPTS})
    _add_compile_lua_command(${LUA_SCRIPT} LUA_OBJS)
endforeach()

add_custom_target(lua_script_objs
    DEPENDS ${LUA_OBJS}
)

add_library(lua_scripts OBJECT IMPORTED GLOBAL)
add_dependencies(lua_scripts lua_script_objs)
set_target_properties(lua_scripts PROPERTIES
    IMPORTED_OBJECTS "${LUA_OBJS}"
)
