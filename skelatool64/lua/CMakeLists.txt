set(EMBED_RESOURCE "${PROJECT_SOURCE_DIR}/cmake/EmbedResource.cmake")

set(LUA_SCRIPTS
    sk_animation.lua
    sk_definition_writer.lua
    sk_math.lua
    sk_scene.lua
)

function(_add_embed_lua_command INPUT_FILE OUTPUT_LIST)
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
    set(OUTPUT_FILE_OBJ "${CMAKE_BINARY_DIR}/${RELATIVE_CURRENT_DIR}/${INPUT_NAME}.out")
    set(OUTPUT_FILE_C "${OUTPUT_FILE_OBJ}.c")

    add_custom_command(
        DEPENDS
            ${EMBED_RESOURCE} ${INPUT_FILE}
        OUTPUT
            ${OUTPUT_FILE_OBJ} ${OUTPUT_FILE_C}
        COMMAND
            ${Luac_EXECUTABLE} -o ${OUTPUT_FILE_OBJ} ${INPUT_FILE}
        COMMAND
            # Use relative path for nice symbol name
            ${CMAKE_COMMAND}
            -D INPUT_FILE=$<PATH:RELATIVE_PATH,${OUTPUT_FILE_OBJ},${CMAKE_BINARY_DIR}>
            -D OUTPUT_FILE=${OUTPUT_FILE_C}
            -P ${EMBED_RESOURCE}
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}
        COMMENT
            "Embedding $<PATH:RELATIVE_PATH,${INPUT_FILE},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    list(APPEND ${OUTPUT_LIST} ${OUTPUT_FILE_C})
    return(PROPAGATE ${OUTPUT_LIST})
endfunction()

set(LUA_C_FILES "")

foreach(LUA_SCRIPT ${LUA_SCRIPTS})
    _add_embed_lua_command(${LUA_SCRIPT} LUA_C_FILES)
endforeach()

add_custom_target(lua_c_files
    DEPENDS ${LUA_C_FILES}
)

add_library(lua_scripts INTERFACE)
add_dependencies(lua_scripts lua_c_files)
target_sources(lua_scripts INTERFACE
    ${LUA_C_FILES}
)
