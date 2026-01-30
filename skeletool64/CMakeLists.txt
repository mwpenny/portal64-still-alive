cmake_minimum_required(VERSION 3.28)
project(skeletool64)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(assimp                 REQUIRED)
find_package(CImg                   REQUIRED)
find_package(Lua        5.4 EXACT   REQUIRED)
find_package(Luac       5.4 EXACT   REQUIRED)
find_package(yaml-cpp   0.8         REQUIRED)
find_package(PNG                    REQUIRED)
find_package(TIFF                   REQUIRED)

if (assimp_FOUND AND (
    ((${assimp_VERSION_MAJOR} EQUAL 5) AND (${assimp_VERSION_MINOR} GREATER_EQUAL 4)) OR
    ((${assimp_VERSION_MAJOR} EQUAL 6) AND (${assimp_VERSION_MINOR} EQUAL 0) AND (${assimp_VERSION_PATCH} LESS 4))
))
    message(SEND_ERROR "Found assimp version '${assimp_VERSION}'. Versions in range [5.4.0, 6.0.4) are buggy and unsupported.")
endif()

add_subdirectory(lua)

add_executable(skeletool64
    main.cpp
    src/BoneHierarchy.cpp
    src/CFileDefinition.cpp
    src/CommandLineParser.cpp
    src/DisplayList.cpp
    src/DisplayListGenerator.cpp
    src/DisplayListSettings.cpp
    src/ErrorResult.cpp
    src/ExtendedMesh.cpp
    src/FileUtils.cpp
    src/MathUtil.cpp
    src/MeshWriter.cpp
    src/RCPState.cpp
    src/RenderChunk.cpp
    src/RenderChunkOrder.cpp
    src/SceneLoader.cpp
    src/SceneModification.cpp
    src/SceneWriter.cpp
    src/StringUtls.cpp
    src/ZSorter.cpp
    src/definition_generator/AnimationGenerator.cpp
    src/definition_generator/CollisionGenerator.cpp
    src/definition_generator/CollisionQuad.cpp
    src/definition_generator/DefinitionGenerator.cpp
    src/definition_generator/MaterialGenerator.cpp
    src/definition_generator/MeshDefinitionGenerator.cpp
    src/definitions/DataChunk.cpp
    src/definitions/FileDefinition.cpp
    src/lua_generator/LuaBasicTypes.cpp
    src/lua_generator/LuaDefinitionWriter.cpp
    src/lua_generator/LuaDisplayListSettings.cpp
    src/lua_generator/LuaGenerator.cpp
    src/lua_generator/LuaGeometry.cpp
    src/lua_generator/LuaMesh.cpp
    src/lua_generator/LuaNodeGroups.cpp
    src/lua_generator/LuaScene.cpp
    src/lua_generator/LuaTransform.cpp
    src/lua_generator/LuaUtils.cpp
    src/lua_generator/LuaYaml.cpp
    src/materials/CImgu8.cpp
    src/materials/CombineMode.cpp
    src/materials/Material.cpp
    src/materials/MaterialEnums.cpp
    src/materials/MaterialParser.cpp
    src/materials/MaterialState.cpp
    src/materials/MaterialTranslator.cpp
    src/materials/RenderMode.cpp
    src/materials/TextureCache.cpp
    src/materials/TextureDefinition.cpp
    src/materials/TextureFormats.cpp
    src/math/LeastSquares.cpp
    src/math/MES.cpp
    src/math/Vector4.cpp
)

target_include_directories(skeletool64 PRIVATE
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(skeletool64 PRIVATE
    assimp::assimp
    cimg::cimg
    PNG::PNG
    TIFF::TIFF
    ${LUA_LIBRARIES}
    yaml-cpp::yaml-cpp
    lua_scripts
)

install(TARGETS skeletool64)
